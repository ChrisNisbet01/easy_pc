#include "cpt_node.h"
#include "easy_pc_private.h"
#include "parsers.h"
#include "result.h"

#include "easy_pc/easy_pc_version.h"

char const *
epc_get_version(void)
{
    return EASY_PC_VERSION;
}

#ifdef WITH_INPUT_STREAM_SUPPORT
#include <ctype.h>
#include <pthread.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#ifdef WITH_INPUT_STREAM_SUPPORT
typedef struct
{
    epc_parser_t * top_parser;
    epc_parser_ctx_t * ctx;
    epc_parse_result_t result;
} ParsingThreadArgs;

static void *
epc_parsing_thread_worker(void * arg)
{
    ParsingThreadArgs * args = (ParsingThreadArgs *)arg;

    args->result = args->top_parser->parse_fn(args->top_parser, args->ctx, 0);

    return NULL;
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

    return true;
}

// Internal parser_ctx_t creation (for parse results)
static epc_parser_ctx_t *
internal_create_parse_ctx_from_buffer(char const * buf, size_t len)
{
    mmap_input_buffer_t buffer = create_mmap_input_buffer(len);

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
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);
#endif

    memcpy(buffer.buffer, buf, len);

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

#ifdef WITH_INPUT_STREAM_SUPPORT
    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);
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

    pthread_mutex_init(&ctx->mutex, NULL);
    pthread_cond_init(&ctx->cond, NULL);

    ctx->mmap_buffer = buffer;
    ctx->input_start = buffer.buffer;
    ctx->input_len = 0;
    ctx->is_streaming = true;

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
    pthread_mutex_destroy(&ctx->mutex);
    pthread_cond_destroy(&ctx->cond);
#endif

    if (ctx->mmap_buffer.buffer != NULL)
    {
        munmap((void *)ctx->mmap_buffer.buffer, ctx->mmap_buffer.total_size);
    }

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
        };
    }

#ifdef WITH_INPUT_STREAM_SUPPORT
    if (ctx->is_streaming)
    {
        pthread_mutex_lock(&ctx->mutex);
        while (input_offset + count > ctx->input_len && !ctx->is_eof && ctx->input_error == 0)
        {
            pthread_cond_wait(&ctx->cond, &ctx->mutex);
        }

        if (input_offset + count > ctx->input_len)
        {
            // We've reached EOF or an error occurred before enough data was available
            pthread_mutex_unlock(&ctx->mutex);
            return (parse_get_input_result_t){
                .next_input = &ctx->input_start[input_offset],
                .available = ctx->input_len - input_offset,
                .is_eof = true,
                /* We might want to pass back the error code somehow in the future */
            };
        }

        parse_get_input_result_t result = {
            .next_input = &ctx->input_start[input_offset],
            .available = ctx->input_len - input_offset,
            .is_eof = false,
        };
        pthread_mutex_unlock(&ctx->mutex);
        return result;
    }
#endif

    if (input_offset + count > ctx->input_len)
    {
        return (parse_get_input_result_t){
            .next_input = &ctx->input_start[input_offset],
            .available = ctx->input_len - input_offset,
            .is_eof = true,
        };
    }

    return (parse_get_input_result_t){
        .next_input = &ctx->input_start[input_offset],
        .available = ctx->input_len - input_offset,
    };
}

EASY_PC_HIDDEN
bool
parse_ctx_is_streaming(epc_parser_ctx_t const * ctx)
{
#ifdef WITH_INPUT_STREAM_SUPPORT
    return ctx ? ctx->is_streaming : false;
#else
    (void)ctx;
    return false;
#endif
}

EASY_PC_HIDDEN
bool
parse_ctx_is_eof(epc_parser_ctx_t * ctx)
{
    if (!ctx)
    {
        return true;
    }
#ifdef WITH_INPUT_STREAM_SUPPORT
    if (ctx->is_streaming)
    {
        pthread_mutex_lock(&ctx->mutex);
        bool const is_eof = ctx->is_eof;
        pthread_mutex_unlock(&ctx->mutex);
        return is_eof;
    }
#endif
    return true; // For non-streaming, data is always loaded up to "EOF"
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

#ifdef WITH_INPUT_STREAM_SUPPORT
static epc_parse_result_t
parse_in_thread(epc_parser_t * top_parser, epc_parser_ctx_t * ctx, epc_parse_input_t input)
{
    ParsingThreadArgs args = {
        .top_parser = top_parser,
        .ctx = ctx,
        .result = {0},
    };

    pthread_t thread;
    if (pthread_create(&thread, NULL, epc_parsing_thread_worker, &args) != 0)
    {
        return epc_unparsed_error_result(
            0, "Failed to create parsing thread", "parsing thread created", "pthread_create failed"
        );
    }

    // Producer Loop (Main Thread)
    char read_buf[4096];
    ssize_t bytes_read;
    while ((bytes_read = read(input.fd, read_buf, sizeof(read_buf))) > 0)
    {
        pthread_mutex_lock(&ctx->mutex);

        // Check if we have space in mmap buffer (100MB limit currently)
        if (ctx->input_len + (size_t)bytes_read > MAX_MMAP_INPUT_SIZE)
        {
            ctx->input_error = EFBIG;
            pthread_cond_broadcast(&ctx->cond);
            pthread_mutex_unlock(&ctx->mutex);
            break;
        }

        memcpy((void *)(ctx->input_start + ctx->input_len), read_buf, (size_t)bytes_read);
        ctx->input_len += (size_t)bytes_read;

        pthread_cond_broadcast(&ctx->cond);
        pthread_mutex_unlock(&ctx->mutex);
    }

    pthread_mutex_lock(&ctx->mutex);
    if (bytes_read == 0)
    {
        ctx->is_eof = true;
    }
    else if (bytes_read < 0)
    {
        ctx->input_error = errno;
    }
    pthread_cond_broadcast(&ctx->cond);
    pthread_mutex_unlock(&ctx->mutex);

    pthread_join(thread, NULL);

    return args.result;
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
    if (ctx->is_streaming)
    {
        session.result = parse_in_thread(top_parser, ctx, input);
    }
    else
#endif
    {
        session.result = top_parser->parse_fn(top_parser, ctx, 0);
    }

    // After parsing, if an error occurred, check if the tracked "furthest_error"
    // is more informative than the one that caused the final failure.
    if (session.result.is_error)
    {
        epc_parser_error_t * furthest_error = parser_furthest_error_copy(ctx);

        // A `furthest_error` is more informative if it parsed further into the input string.
        if (furthest_error != NULL
            && (session.result.data.error == NULL
                || furthest_error->input_position > session.result.data.error->input_position))
        {
            // If it is, replace the result's error with the furthest one.
            epc_parser_result_cleanup(&session.result);
            session.result.is_error = true;
            session.result.data.error = furthest_error;
        }
        else
        {
            // Otherwise, the original error is fine, so just free the copy of furthest_error.
            epc_parser_error_free(furthest_error);
        }
    }

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

EASY_PC_API void
epc_parse_session_destroy(epc_parse_session_t * session)
{
    if (session == NULL)
    {
        return;
    }

    epc_parser_result_cleanup(&session->result);

    if (session->internal_parse_ctx)
    {
        internal_destroy_parse_ctx(session->internal_parse_ctx);
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

EASY_PC_API epc_parser_t *
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
