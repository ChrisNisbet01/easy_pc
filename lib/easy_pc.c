#include "easy_pc/easy_pc_version.h"

#include "cpt_node.h"
#include "easy_pc_private.h"
#include "parsers.h"
#include "result.h"

char const *
epc_get_version(void)
{
    return EASY_PC_VERSION;
}

#ifdef WITH_INPUT_STREAM_SUPPORT
#include <ctype.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef WITH_INPUT_STREAM_SUPPORT
static void *
epc_parsing_thread_worker(void * arg)
{
    epc_parsing_thread_args_t * args = (epc_parsing_thread_args_t *)arg;
    epc_parser_ctx_t * ctx = args->ctx;
    epc_streaming_complete_cb on_complete = ctx->streaming.on_complete;
    void * cb_user_data = ctx->streaming.cb_user_data;
    epc_parse_result_t parse_result = args->top_parser->parse_fn(args->top_parser, ctx, 0);

    pthread_mutex_lock(&ctx->streaming.mutex);

    ctx->streaming.parsing_thread_active = false;
    ctx->streaming.pending_result = parse_result;

    pthread_mutex_unlock(&ctx->streaming.mutex);

    if (on_complete != NULL)
    {
        on_complete(cb_user_data);
    }

    return NULL;
}

static bool
internal_streaming_init_thread(
    epc_parser_ctx_t * ctx,
    epc_parser_t * top_parser,
    int fd,
    epc_streaming_complete_cb on_complete,
    void * cb_user_data
)
{
    ctx->streaming.fd = fd;
    ctx->streaming.on_complete = on_complete;
    ctx->streaming.cb_user_data = cb_user_data;

    ctx->streaming.thread_args.top_parser = top_parser;
    ctx->streaming.thread_args.ctx = ctx;

    ctx->streaming.parsing_thread_active = true;
    ctx->streaming.parsing_thread_joined = false;

    if (pthread_create(&ctx->streaming.parsing_thread, NULL, epc_parsing_thread_worker, &ctx->streaming.thread_args)
        != 0)
    {
        ctx->streaming.parsing_thread_active = false;
        return false;
    }
    return true;
}

static void
internal_streaming_join_thread(epc_parser_ctx_t * ctx)
{
    pthread_t thread = 0;
    bool should_join = false;

    pthread_mutex_lock(&ctx->streaming.mutex);
    if (ctx->streaming.is_streaming && !ctx->streaming.parsing_thread_joined)
    {
        thread = ctx->streaming.parsing_thread;
        should_join = true;
        ctx->streaming.parsing_thread_joined = true;
    }
    pthread_mutex_unlock(&ctx->streaming.mutex);

    if (should_join)
    {
        pthread_join(thread, NULL);
    }
}
#endif

// --- Top-Level API ---

mmap_input_buffer_t
create_mmap_input_buffer(size_t input_size)
{
    mmap_input_buffer_t buffer = {0};

    if (input_size > MAX_MMAP_INPUT_SIZE)
    {
        return buffer; // Return empty buffer if input size exceeds our limit
    }

    long const page_size = sysconf(_SC_PAGESIZE);
    buffer.total_size = MAX_MMAP_INPUT_SIZE + page_size;

    /*
     * Allocate 100MB + 1 guard page.
     * MAP_ANONYMOUS ensures no physical memory is used until written to.
     */
    void * mem = mmap(NULL, buffer.total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED)
    {
        return (mmap_input_buffer_t){0}; // Return empty buffer on failure
    }

    /* Set the guard page at the very end of the 100MB range. */
    if (mprotect((char *)mem + MAX_MMAP_INPUT_SIZE, page_size, PROT_NONE) != 0)
    {
        munmap(mem, buffer.total_size);
        return (mmap_input_buffer_t){0}; // Return empty buffer on failure
    }

    buffer.buffer = mem;
    buffer.input_size = input_size;

    return buffer;
}

static void
node_pool_append_node(epc_node_pool_t * pool, epc_cpt_node_t * node)
{
    if (pool->count == pool->capacity)
    {
        size_t const new_capacity = pool->capacity == 0 ? 1024 : pool->capacity * 2;
        epc_cpt_node_t ** new_nodes = realloc(pool->nodes, new_capacity * sizeof(*new_nodes));
        if (new_nodes == NULL)
        {
            /* If we can't store it in free nodes, we just leak it in the arena. */
            return;
        }
        pool->nodes = new_nodes;
        pool->capacity = new_capacity;
    }

    pool->nodes[pool->count++] = node;
}

static epc_cpt_node_t *
node_pool_pull_node(epc_node_pool_t * pool)
{
    if (pool->count > 0)
    {
        return pool->nodes[--pool->count];
    }

    return NULL;
}

static void
error_pool_append_error(epc_error_pool_t * pool, epc_parser_error_t * error)
{
    if (pool->count == pool->capacity)
    {
        size_t const new_capacity = pool->capacity == 0 ? 1024 : pool->capacity * 2;
        epc_parser_error_t ** new_errors = realloc(pool->errors, new_capacity * sizeof(*new_errors));
        if (new_errors == NULL)
        {
            /* If we can't store it in free errors, we just leak it in the arena. */
            return;
        }
        pool->errors = new_errors;
        pool->capacity = new_capacity;
    }

    pool->errors[pool->count++] = error;
}

static epc_parser_error_t *
error_pool_pull_error(epc_error_pool_t * pool)
{
    if (pool->count > 0)
    {
        return pool->errors[--pool->count];
    }

    return NULL;
}

static void
node_pool_cleanup(epc_node_pool_t * pool)
{
    if (pool->nodes != NULL)
    {
        /* Don't free the nodes stored in the array as this would result in them getting added to the
         * array again.
         */
        free(pool->nodes);
        pool->nodes = NULL;
    }
    pool->count = 0;
    pool->capacity = 0;
}

static void
error_pool_cleanup(epc_error_pool_t * pool)
{
    if (pool->errors != NULL)
    {
        free(pool->errors);
        pool->errors = NULL;
    }
    pool->count = 0;
    pool->capacity = 0;
}

static void
scan_newlines(epc_parser_ctx_t * ctx, char const * input_start, size_t len)
{
    if (ctx == NULL || input_start == NULL || len == 0)
    {
        return;
    }

    size_t base_offset = ctx->newline.count > 0 ? ctx->newline.positions[ctx->newline.count - 1] + 1 : 0;

    for (size_t i = 0; i < len; i++)
    {
        if (input_start[i] == '\n')
        {
            if (ctx->newline.count >= ctx->newline.capacity)
            {
                size_t new_capacity = ctx->newline.capacity * 2;
                size_t * new_positions = realloc(ctx->newline.positions, new_capacity * sizeof(*new_positions));
                if (new_positions == NULL)
                {
                    return;
                }
                ctx->newline.positions = new_positions;
                ctx->newline.capacity = new_capacity;
            }

            ctx->newline.positions[ctx->newline.count++] = base_offset + i;
        }
    }
}

static bool
internal_init_parse_ctx(epc_parser_ctx_t * ctx)
{
    if (ctx == NULL)
    {
        return false;
    }

    ctx->node_arena = epc_arena_create(MAX_NODE_ARENA_SIZE);
    if (ctx->node_arena.base == NULL)
    {
        return false;
    }

    ctx->node_pool.capacity = 1024;
    ctx->node_pool.nodes = calloc(ctx->node_pool.capacity, sizeof(*ctx->node_pool.nodes));
    if (ctx->node_pool.nodes == NULL)
    {
        epc_arena_destroy(&ctx->node_arena);
        return false;
    }

    ctx->error_pool.capacity = 128;
    ctx->error_pool.errors = calloc(ctx->error_pool.capacity, sizeof(*ctx->error_pool.errors));
    if (ctx->error_pool.errors == NULL)
    {
        node_pool_cleanup(&ctx->node_pool);
        epc_arena_destroy(&ctx->node_arena);
        return false;
    }

    ctx->newline.positions = calloc(128, sizeof(*ctx->newline.positions));
    ctx->newline.capacity = 128;
    ctx->newline.count = 0;

    return true;
}

// Internal parser_ctx_t creation (for parse results)
static epc_parser_ctx_t *
internal_create_parse_ctx_from_buffer(char const * buf, size_t len)
{
    mmap_input_buffer_t buffer = create_mmap_input_buffer(len + 1);

    if (buffer.buffer == NULL)
    {
        return NULL;
    }

    epc_parser_ctx_t * ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
    {
        munmap(buffer.buffer, buffer.total_size);
        return NULL;
    }

    if (!internal_init_parse_ctx(ctx))
    {
        munmap(buffer.buffer, buffer.total_size);
        free(ctx);
        return NULL;
    }

#ifdef WITH_INPUT_STREAM_SUPPORT
    pthread_mutex_init(&ctx->streaming.mutex, NULL);
    pthread_cond_init(&ctx->streaming.cond, NULL);
#endif

    memcpy(buffer.buffer, buf, len);
    buffer.buffer[len] = '\0'; // Nul-terminate the buffer

    scan_newlines(ctx, buffer.buffer, len);

    ctx->mmap_buffer = buffer;
    ctx->input_start = ctx->mmap_buffer.buffer;
    ctx->input_len = len;

    return ctx;
}

static epc_parser_ctx_t *
internal_create_parse_ctx_from_string(char const * input_start)
{
    size_t input_len = input_start == NULL ? 0 : strlen(input_start);
    char const * start = input_start == NULL ? "" : input_start;

    epc_parser_ctx_t * ctx = internal_create_parse_ctx_from_buffer(start, input_len + 1);
    if (ctx == NULL)
    {
        return NULL;
    }

    if (start == NULL)
    {
        ctx->mmap_buffer.buffer[0] = '\0'; /* Ensure nul termination for NULL input */
    }

    /* Don't include the NUL terminator in the input length. */
    ctx->input_len = input_len;

    return ctx;
}

static epc_parser_ctx_t *
internal_create_parse_ctx_from_fp(FILE * fp)
{
    if (fp == NULL)
    {
        return NULL;
    }

    // Move to end of file to determine size
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        return NULL;
    }
    long file_size = ftell(fp);
    if (file_size < 0)
    {
        return NULL;
    }
    rewind(fp);

    mmap_input_buffer_t buffer = create_mmap_input_buffer((size_t)file_size + 1);
    if (buffer.buffer == NULL)
    {
        return NULL;
    }

    size_t total_read = fread(buffer.buffer, 1, (size_t)file_size, fp);
    if (total_read != (size_t)file_size)
    {
        munmap(buffer.buffer, buffer.total_size);
        return NULL;
    }
    buffer.buffer[total_read] = '\0'; // Null-terminate the buffer

    epc_parser_ctx_t * ctx = calloc(1, sizeof(*ctx));
    if (!ctx)
    {
        munmap(buffer.buffer, buffer.total_size);
        return NULL;
    }

    if (!internal_init_parse_ctx(ctx))
    {
        munmap(buffer.buffer, buffer.total_size);
        free(ctx);
        return NULL;
    }

    scan_newlines(ctx, buffer.buffer, total_read);

#ifdef WITH_INPUT_STREAM_SUPPORT
    pthread_mutex_init(&ctx->streaming.mutex, NULL);
    pthread_cond_init(&ctx->streaming.cond, NULL);
#endif

    ctx->mmap_buffer = buffer;
    ctx->input_start = buffer.buffer;
    ctx->input_len = total_read;

    return ctx;
}

#ifdef WITH_INPUT_STREAM_SUPPORT
static epc_parser_ctx_t *
internal_create_parse_ctx_streaming(void)
{
    mmap_input_buffer_t buffer = create_mmap_input_buffer(0);
    if (buffer.buffer == NULL)
    {
        return NULL;
    }

    epc_parser_ctx_t * ctx = calloc(1, sizeof(*ctx));
    if (ctx == NULL)
    {
        munmap(buffer.buffer, buffer.total_size);
        return NULL;
    }

    if (!internal_init_parse_ctx(ctx))
    {
        munmap(buffer.buffer, buffer.total_size);
        free(ctx);
        return NULL;
    }

    pthread_mutex_init(&ctx->streaming.mutex, NULL);
    pthread_cond_init(&ctx->streaming.cond, NULL);

    buffer.buffer[0] = '\0'; /* NUL-terminate the buffer. */
    ctx->mmap_buffer = buffer;
    ctx->input_start = buffer.buffer;
    ctx->input_len = 0;
    ctx->streaming.is_streaming = true;

    return ctx;
}
#endif

EASY_PC_API
void *
parse_ctx_get_user_ctx(epc_parser_ctx_t const * ctx)
{
    return ctx ? ctx->user_ctx : NULL;
}

EASY_PC_HIDDEN
epc_memo_table_t *
epc_parser_ctx_get_memo_table(epc_parser_ctx_t * ctx)
{
    return ctx ? &ctx->memo_table : NULL;
}

// Internal parser_ctx_t destruction (for parse results)
static void
internal_destroy_parse_ctx(epc_parser_ctx_t * ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    epc_parser_error_free(ctx->furthest_error);

#if INCLUDE_MEMOIZATION_DEBUG
    /*
     *   Be careful not to clean up the parsers before printing this table, as it contains pointers to
     *   memory owned by them.
     */
    epc_memo_table_print(ctx);
#endif
    epc_memo_table_cleanup(ctx);

#ifdef WITH_INPUT_STREAM_SUPPORT
    pthread_mutex_destroy(&ctx->streaming.mutex);
    pthread_cond_destroy(&ctx->streaming.cond);
    epc_parser_result_cleanup(&ctx->streaming.pending_result);
#endif

    if (ctx->mmap_buffer.buffer != NULL)
    {
        munmap((void *)ctx->mmap_buffer.buffer, ctx->mmap_buffer.total_size);
    }

    free(ctx->newline.positions);

    epc_arena_destroy(&ctx->node_arena);
    node_pool_cleanup(&ctx->node_pool);
    error_pool_cleanup(&ctx->error_pool);

    free(ctx);
}

EASY_PC_HIDDEN
epc_cpt_node_t *
parse_ctx_alloc_node(epc_parser_ctx_t * ctx)
{
    epc_cpt_node_t * node;

    if (ctx == NULL)
    {
        node = calloc(1, sizeof(*node));

        return node;
    }

    /* Pull from previously allocated nodes first. */
    node = node_pool_pull_node(&ctx->node_pool);

    /* Resort to the arena if the pool is empty. */
    if (node == NULL)
    {
        node = epc_arena_alloc(&ctx->node_arena, sizeof(epc_cpt_node_t));
    }

    if (node != NULL)
    {
        memset(node, 0, sizeof(*node));
    }

    return node;
}

EASY_PC_HIDDEN
void
parse_ctx_free_node(epc_parser_ctx_t * ctx, epc_cpt_node_t * node)
{
    if (node == NULL)
    {
        return;
    }

    if (ctx == NULL)
    {
        free(node);
        return;
    }

    node_pool_append_node(&ctx->node_pool, node);
}

EASY_PC_HIDDEN
epc_parser_error_t *
parse_ctx_alloc_error(epc_parser_ctx_t * ctx)
{
    epc_parser_error_t * error;

    if (ctx == NULL)
    {
        error = calloc(1, sizeof(*error));

        return error;
    }

    /* Pull from previously allocated errors first. */
    error = error_pool_pull_error(&ctx->error_pool);

    /* Resort to the arena if the pool is empty. */
    if (error == NULL)
    {
        error = epc_arena_alloc(&ctx->node_arena, sizeof(epc_parser_error_t));
    }

    if (error != NULL)
    {
        memset(error, 0, sizeof(*error));
        error->internal_parse_ctx = ctx;
    }

    return error;
}

EASY_PC_HIDDEN
void
parse_ctx_free_error(epc_parser_ctx_t * ctx, epc_parser_error_t * error)
{
    if (error == NULL)
    {
        return;
    }

    if (ctx == NULL)
    {
        free(error);
        return;
    }

    error_pool_append_error(&ctx->error_pool, error);
}

EASY_PC_HIDDEN
parse_get_input_result_t
parse_ctx_get_input_at_offset(epc_parser_ctx_t * const ctx, size_t const input_offset, size_t const count)
{
    if (ctx == NULL || ctx->input_start == NULL)
    {
        return (parse_get_input_result_t){
            .is_eof = true,
            .had_error = true,
        };
    }

    bool const is_streaming = ctx->streaming.is_streaming;
    bool had_error = false;

#ifdef WITH_INPUT_STREAM_SUPPORT
    if (is_streaming)
    {
        /* Wait for the input to arrive from the main thread. */
        pthread_mutex_lock(&ctx->streaming.mutex);
        while (input_offset + count > ctx->input_len && !ctx->streaming.is_eof && ctx->streaming.input_error == 0)
        {
            pthread_cond_wait(&ctx->streaming.cond, &ctx->streaming.mutex);
        }
        if (ctx->streaming.input_error != 0)
        {
            had_error = true;
        }
    }
#endif

    /* Note that the streaming mutex is still held at this point. */
    bool const is_eof = input_offset + count > ctx->input_len;

    parse_get_input_result_t result = {
        .next_input = &ctx->input_start[input_offset],
        .available = ctx->input_len - input_offset,
        .is_eof = is_eof,
        .had_error = had_error,
    };

#ifdef WITH_INPUT_STREAM_SUPPORT
    if (is_streaming)
    {
        pthread_mutex_unlock(&ctx->streaming.mutex);
    }
#endif

    return result;
}

EASY_PC_HIDDEN
size_t
parse_ctx_get_input_len(epc_parser_ctx_t * const ctx)
{
    if (ctx == NULL || ctx->input_start == NULL)
    {
        return 0;
    }
    return ctx->input_len;
}

EASY_PC_HIDDEN
ATTR_NONNULL(1)
size_t
parse_ctx_get_offset_from_input(epc_parser_ctx_t * const ctx, char const * const input_position)
{
    if (ctx->input_start == NULL || input_position < ctx->input_start
        || input_position > ctx->input_start + ctx->input_len)
    {
        return 0;
    }
    return (size_t)(input_position - ctx->input_start);
}

EASY_PC_HIDDEN
ATTR_NONNULL(1)
epc_parser_error_t *
parse_ctx_get_furthest_error(epc_parser_ctx_t const * ctx)
{
    return ctx->furthest_error;
}

EASY_PC_HIDDEN
ATTR_NONNULL(1)
void
parser_ctx_set_furthest_error(epc_parser_ctx_t * ctx, epc_parser_error_t ** replacement)
{
    epc_parser_error_free(ctx->furthest_error);
    ctx->furthest_error = *replacement;
    *replacement = NULL;
}

static void
choose_best_error(epc_parse_session_t * session)
{
    epc_parser_ctx_t * ctx = session->internal_parse_ctx;

    // After parsing, if an error occurred, check if the tracked "furthest_error"
    // is more informative than the one that caused the final failure.
    if (session->result.is_error)
    {
        epc_parser_error_t * furthest_error = parser_furthest_error_copy(ctx);

        // A `furthest_error` is more informative if it parsed further into the input string.
        if (furthest_error != NULL
            && (session->result.data.error == NULL
                || furthest_error->input_position > session->result.data.error->input_position))
        {
            // If it is, replace the result's error with the furthest one.
            epc_parser_error_free(session->result.data.error);
            session->result.data.error = furthest_error;
        }
        else
        {
            // Otherwise, the original error is fine, so just free the copy of furthest_error.
            epc_parser_error_free(furthest_error);
        }
    }
}

#ifdef WITH_INPUT_STREAM_SUPPORT
typedef enum
{
    STREAM_CONSUME_OK,
    STREAM_CONSUME_EOF,
    STREAM_CONSUME_ERROR,
    STREAM_CONSUME_AGAIN
} stream_consume_status_t;

static stream_consume_status_t
internal_streaming_consume_available(epc_parse_session_t * session, bool once)
{
    epc_parser_ctx_t * ctx = session->internal_parse_ctx;
    int fd = ctx->streaming.fd;
    char read_buf[4096];
    ssize_t bytes_read;

    do
    {
        bytes_read = read(fd, read_buf, sizeof(read_buf));
        if (bytes_read > 0)
        {
            pthread_mutex_lock(&ctx->streaming.mutex);
            if (ctx->input_len + (size_t)bytes_read + 1 > MAX_MMAP_INPUT_SIZE)
            {
                ctx->streaming.input_error = EFBIG;
                pthread_cond_broadcast(&ctx->streaming.cond);
                pthread_mutex_unlock(&ctx->streaming.mutex);
                return STREAM_CONSUME_ERROR;
            }
            memcpy((void *)(ctx->input_start + ctx->input_len), read_buf, (size_t)bytes_read);
            /* NUL terminate to help prevent buffer overruns within the parsers. */
            *(uint8_t *)&ctx->input_start[ctx->input_len + bytes_read] = '\0';
            scan_newlines(ctx, ctx->input_start + ctx->input_len, (size_t)bytes_read);
            ctx->input_len += (size_t)bytes_read;
            pthread_cond_broadcast(&ctx->streaming.cond);
            pthread_mutex_unlock(&ctx->streaming.mutex);
        }
        else if (bytes_read == 0)
        {
            epc_streaming_notify_eof(session);
            return STREAM_CONSUME_EOF;
        }
        else if (errno == EINTR)
        {
            /* Interrupted by signal, retry immediately. */
            continue;
        }
        else if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return STREAM_CONSUME_AGAIN;
        }
        else
        {
            epc_streaming_notify_error(session, errno);
            return STREAM_CONSUME_ERROR;
        }
    } while (!once && bytes_read > 0);

    return STREAM_CONSUME_OK;
}

EASY_PC_API void
epc_streaming_notify_readable(epc_parse_session_t * session)
{
    if (session == NULL || session->internal_parse_ctx == NULL)
    {
        return;
    }

    internal_streaming_consume_available(session, true);
}

EASY_PC_API void
epc_streaming_notify_eof(epc_parse_session_t * session)
{
    if (session == NULL || session->internal_parse_ctx == NULL)
    {
        return;
    }

    epc_parser_ctx_t * ctx = session->internal_parse_ctx;
    pthread_mutex_lock(&ctx->streaming.mutex);
    ctx->streaming.is_eof = true;
    pthread_cond_broadcast(&ctx->streaming.cond);
    pthread_mutex_unlock(&ctx->streaming.mutex);
}

EASY_PC_API void
epc_streaming_notify_error(epc_parse_session_t * session, int error_code)
{
    if (session == NULL || session->internal_parse_ctx == NULL)
    {
        return;
    }

    epc_parser_ctx_t * ctx = session->internal_parse_ctx;
    pthread_mutex_lock(&ctx->streaming.mutex);
    ctx->streaming.input_error = error_code;
    pthread_cond_broadcast(&ctx->streaming.cond);
    pthread_mutex_unlock(&ctx->streaming.mutex);
}

EASY_PC_API bool
epc_parse_session_advance(epc_parse_session_t * session, epc_parser_t * next_parser)
{
    if (session == NULL || session->internal_parse_ctx == NULL || next_parser == NULL)
    {
        return false;
    }

    epc_parser_ctx_t * ctx = session->internal_parse_ctx;

    /* 1. Ensure the previous thread is joined. */
    internal_streaming_join_thread(ctx);

    /* Can't advance if the input file is closed or experienced and error. */
    if (!ctx->streaming.is_streaming || ctx->streaming.is_eof || ctx->streaming.input_error != 0)
    {
        return false;
    }

    /* 2. Determine how much was consumed. */
    size_t consumed = 0;

    if (!session->result.is_error)
    {
        if (session->result.data.success != NULL)
        {
            consumed = epc_cpt_node_get_len(session->result.data.success);
        }
    }
    else
    {
        consumed = 1; /* Consume at least one byte, else the parser will likely loop forever, consuming no input. */
    }

    pthread_mutex_lock(&ctx->streaming.mutex);

    /* 3. Compact the buffer. */
    if (consumed > 0 && consumed <= ctx->input_len)
    {
        size_t leftover = ctx->input_len - consumed;
        if (leftover > 0)
        {
            memmove((void *)ctx->input_start, ctx->input_start + consumed, leftover);
        }
        ctx->input_len = leftover;
    }

    /* 4. Reset internal state. */
    epc_parser_result_cleanup(&session->result);
    epc_parser_result_cleanup(&ctx->streaming.pending_result);
    epc_parser_error_free(ctx->furthest_error);
    ctx->furthest_error = NULL;

    epc_memo_table_reset(ctx);

    /* 5. Prepare and restart the consumer thread. */
    if (!internal_streaming_init_thread(
            ctx, next_parser, ctx->streaming.fd, ctx->streaming.on_complete, ctx->streaming.cb_user_data
        ))
    {
        session->result = epc_unparsed_error_result(
            0, "Failed to restart parsing thread", "parsing thread restarted", "pthread_create failed"
        );
    }

    pthread_mutex_unlock(&ctx->streaming.mutex);

    return true;
}

EASY_PC_API bool
epc_parse_session_is_active(epc_parse_session_t const * session)
{
    if (session == NULL || session->internal_parse_ctx == NULL)
    {
        return false;
    }

    epc_parser_ctx_t * ctx = session->internal_parse_ctx;
    pthread_mutex_lock(&ctx->streaming.mutex);
    bool const active = ctx->streaming.parsing_thread_active;
    pthread_mutex_unlock(&ctx->streaming.mutex);
    return active;
}

EASY_PC_API bool
epc_parse_session_sync_result(epc_parse_session_t * session)
{
    if (session == NULL || session->internal_parse_ctx == NULL)
    {
        return false;
    }

    epc_parser_ctx_t * ctx = session->internal_parse_ctx;

    epc_parser_result_cleanup(&session->result);

    pthread_mutex_lock(&ctx->streaming.mutex);

    /* Move result from internal storage to session. */
    session->result = ctx->streaming.pending_result;
    memset(&ctx->streaming.pending_result, 0, sizeof(ctx->streaming.pending_result));

    choose_best_error(session);

    pthread_mutex_unlock(&ctx->streaming.mutex);

    return true;
}
#endif

EASY_PC_HIDDEN epc_parse_session_t
epc_parse_input(epc_parser_t * top_parser, epc_parse_input_t input, void * user_ctx)
{
    epc_parse_session_t session = {0};

    if (top_parser == NULL)
    {
        session.result = epc_unparsed_error_result(
            0, "Top parser not set for grammar", "grammar with a top parser", "NULL top_parser"
        );
        return session;
    }

    epc_parser_ctx_t * ctx = NULL;

    switch (input.type)
    {
    case EPC_PARSE_TYPE_STRING:
        if (input.input_string == NULL)
        {
            session.result = epc_unparsed_error_result(0, "Input string is NULL", "non-NULL input string", "NULL");
            return session;
        }
        ctx = internal_create_parse_ctx_from_string(input.input_string);
        break;

    case EPC_PARSE_TYPE_FILE:
        if (input.fp == NULL)
        {
            session.result = epc_unparsed_error_result(0, "Input file is NULL", "non-NULL input file", "NULL");
            return session;
        }
        ctx = internal_create_parse_ctx_from_fp(input.fp);
        break;

    case EPC_PARSE_TYPE_FILENAME:
        if (input.filename == NULL)
        {
            session.result = epc_unparsed_error_result(0, "Input filename is NULL", "non-NULL input filename", "NULL");
            return session;
        }
        FILE * fp = fopen(input.filename, "r");
        if (fp == NULL)
        {
            char error_message[256];
            snprintf(
                error_message, sizeof(error_message), "Failed to open file '%s': %s", input.filename, strerror(errno)
            );
            session.result = epc_unparsed_error_result(0, error_message, "file that can be opened", "unopenable file");
            return session;
        }
        ctx = internal_create_parse_ctx_from_fp(fp);
        fclose(fp);

        break;

    case EPC_PARSE_TYPE_BUFFER:
        if (input.buffer.buf == NULL)
        {
            session.result = epc_unparsed_error_result(0, "Input buffer is NULL", "non-NULL input buffer", "NULL");
            return session;
        }
        ctx = internal_create_parse_ctx_from_buffer(input.buffer.buf, input.buffer.len);
        break;

    case EPC_PARSE_TYPE_FD:
    case EPC_PARSE_TYPE_FD_REACTIVE:
    {
#ifdef WITH_INPUT_STREAM_SUPPORT
        ctx = internal_create_parse_ctx_streaming();
        break;
#else
        session.result = epc_unparsed_error_result(
            0,
            "Streaming input not supported in this build",
            "build with streaming input support",
            "no streaming support"
        );
        return session;
#endif
    }

    default:
        session.result = epc_unparsed_error_result(0, "Invalid input type", "valid input type", "invalid input type");
        return session;
    }

    if (ctx == NULL)
    {
        session.result = epc_unparsed_error_result(0, "Failed to create parse context.", "valid parse context", "NULL");
        return session;
    }
    session.internal_parse_ctx = ctx;
    ctx->user_ctx = user_ctx;

#ifdef WITH_INPUT_STREAM_SUPPORT
    if (ctx->streaming.is_streaming)
    {
        int fd;
        epc_streaming_complete_cb on_complete = NULL;
        void * cb_user_data = NULL;

        if (input.type == EPC_PARSE_TYPE_FD)
        {
            fd = input.fd;
        }
        else
        {
            fd = input.reactive.fd;
            on_complete = input.reactive.on_complete;
            cb_user_data = input.reactive.cb_user_data;
        }

        /* Ensure the FD is in non-blocking mode. */
        int flags = fcntl(fd, F_GETFL, 0);
        if (flags != -1)
        {
            fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        }

        if (!internal_streaming_init_thread(ctx, top_parser, fd, on_complete, cb_user_data))
        {
            session.result = epc_unparsed_error_result(
                0, "Failed to create parsing thread", "parsing thread created", "pthread_create failed"
            );
            return session;
        }

        if (input.type == EPC_PARSE_TYPE_FD)
        {
            /* Blocking producer loop. */
            bool done_producing = false;
            while (!done_producing)
            {
                stream_consume_status_t status = internal_streaming_consume_available(&session, false);

                pthread_mutex_lock(&ctx->streaming.mutex);
                if (ctx->streaming.is_eof || ctx->streaming.input_error != 0 || !ctx->streaming.parsing_thread_active)
                {
                    done_producing = true;
                }
                pthread_mutex_unlock(&ctx->streaming.mutex);

                if (!done_producing && status == STREAM_CONSUME_AGAIN)
                {
                    struct pollfd pfd = {.fd = fd, .events = POLLIN};
                    poll(&pfd, 1, 100);
                }
                else if (status == STREAM_CONSUME_ERROR)
                {
                    done_producing = true;
                }
            }

            /* Wait for the consumer thread to finish. */
            internal_streaming_join_thread(ctx);

            /* Move result from internal storage to session. */
            epc_parse_session_sync_result(&session);
        }
        else
        {
            /* Reactive mode - return session immediately. */
            return session;
        }
    }
    else
#endif
    {
        session.result = top_parser->parse_fn(top_parser, ctx, 0);
    }

    choose_best_error(&session);

    return session;
}

EASY_PC_API epc_parse_session_t
epc_parse_str(epc_parser_t * top_parser, char const * input_string, void * user_ctx)
{
    epc_parse_input_t input = {.type = EPC_PARSE_TYPE_STRING, .input_string = input_string};

    return epc_parse_input(top_parser, input, user_ctx);
}

EASY_PC_API epc_parse_session_t
epc_parse_fp(epc_parser_t * top_parser, FILE * fp, void * user_ctx)
{
    epc_parse_input_t input = {.type = EPC_PARSE_TYPE_FILE, .fp = fp};

    return epc_parse_input(top_parser, input, user_ctx);
}

EASY_PC_API epc_parse_session_t
epc_parse_file(epc_parser_t * top_parser, char const * filename, void * user_ctx)
{
    epc_parse_input_t input = {.type = EPC_PARSE_TYPE_FILENAME, .filename = filename};

    return epc_parse_input(top_parser, input, user_ctx);
}

EASY_PC_API epc_parse_session_t
epc_parse_fd(epc_parser_t * top_parser, int fd, void * user_ctx)
{
    epc_parse_input_t input = {.type = EPC_PARSE_TYPE_FD, .fd = fd};

    return epc_parse_input(top_parser, input, user_ctx);
}

EASY_PC_API epc_parse_session_t
epc_parse_bytes(epc_parser_t * top_parser, char const * buf, size_t len, void * user_ctx)
{
    epc_parse_input_t input = {.type = EPC_PARSE_TYPE_BUFFER, .buffer = {.buf = buf, .len = len}};
    return epc_parse_input(top_parser, input, user_ctx);
}

EASY_PC_API epc_parse_session_t
epc_parse_fd_reactive(
    epc_parser_t * top_parser, int fd, epc_streaming_complete_cb on_complete, void * cb_user_data, void * user_ctx
)
{
    epc_parse_input_t input = {
        .type = EPC_PARSE_TYPE_FD_REACTIVE,
        .reactive = {
            .fd = fd,
            .on_complete = on_complete,
            .cb_user_data = cb_user_data,
        },
    };

    return epc_parse_input(top_parser, input, user_ctx);
}

EASY_PC_API void
epc_parse_session_destroy(epc_parse_session_t * session)
{
    if (session == NULL)
    {
        return;
    }

    if (session->internal_parse_ctx)
    {
        epc_parser_ctx_t * ctx = session->internal_parse_ctx;

#ifdef WITH_INPUT_STREAM_SUPPORT
        internal_streaming_join_thread(ctx);
#endif
        epc_parser_result_cleanup(&session->result);
        internal_destroy_parse_ctx(ctx);
        session->internal_parse_ctx = NULL;
    }
}

EASY_PC_API
void
epc_parse_session_print_cpt(FILE * fp, epc_parse_session_t const * session)
{
    if (session == NULL)
    {
        fprintf(fp, "NULL session\n");
        return;
    }
    if (session->result.is_error)
    {
        epc_parser_error_t * err = session->result.data.error;
        fprintf(fp, "Parse Error: %s\n", err->message);
        fprintf(fp, "At line %zu, col %zu\n", err->position.line + 1, err->position.col + 1);
        fprintf(fp, "Expected: %s\n", err->expected);
        fprintf(fp, "Found: %s\n", err->found);
    }
    else
    {
        fprintf(fp, "Parsing successful!\n");
        char * cpt_str = epc_cpt_to_string(session->internal_parse_ctx, session->result.data.success);
        if (cpt_str)
        {
            fprintf(fp, "Concrete Parse Tree (CPT):\n%s\n", cpt_str);
            free(cpt_str);
        }
    }
}

EASY_PC_API epc_parser_list *
epc_parser_list_create(void)
{
    epc_parser_list * list = calloc(1, sizeof(*list));
    if (!list)
    {
        return NULL;
    }

    list->capacity = 20; // Initial capacity
    list->parsers = calloc(list->capacity, sizeof(*list->parsers));
    if (!list->parsers)
    {
        free(list);
        return NULL;
    }

    list->count = 0;
    return list;
}

EASY_PC_HIDDEN epc_parser_t *
epc_parser_list_add(epc_parser_list * list, epc_parser_t * parser)
{
    if (!list || !parser)
    {
        return NULL; // Return NULL if list or parser is NULL
    }

    if (list->count == list->capacity)
    {
        size_t new_capacity = list->capacity * 2;
        epc_parser_t ** new_parsers = realloc(list->parsers, new_capacity * sizeof(*new_parsers));
        if (!new_parsers)
        {
            epc_parser_free(parser);
            return NULL; // Reallocation failed
        }
        list->parsers = new_parsers;
        list->capacity = new_capacity;
    }

    list->parsers[list->count++] = parser;
    return parser;
}

EASY_PC_API void
epc_parser_list_free(epc_parser_list * list)
{
    if (!list)
    {
        return;
    }

    for (size_t i = 0; i < list->count; ++i)
    {
        epc_parser_free(list->parsers[i]);
    }

    free(list->parsers);
    free(list);
}

static void
pt_visit_recursive(epc_cpt_node_t * node, epc_cpt_visitor_t * visitor)
{
    if (!node || !visitor)
    {
        return;
    }

    if (visitor->enter_node)
    {
        visitor->enter_node(node, visitor->user_data);
    }
    for (int i = 0; i < node->children_count; ++i)
    {
        pt_visit_recursive(node->children[i], visitor);
    }

    if (visitor->exit_node)
    {
        visitor->exit_node(node, visitor->user_data);
    }
}

EASY_PC_API void
epc_cpt_visit_nodes(epc_cpt_node_t * root, epc_cpt_visitor_t * visitor)
{
    if (!root || !visitor)
    {
        return;
    }
    pt_visit_recursive(root, visitor);
}
