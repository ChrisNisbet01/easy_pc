#include "token.h"

#include <easy_pc/easy_pc.h>

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define EPC_TOKEN_LIST_MAX_SIZE (100 * 1024 * 1024) /* 100 MB */

struct epc_token_list_t
{
    epc_parser_token_t * tokens;
    size_t count;
    size_t capacity;
    void * mmap_base;
    size_t mmap_size;
};

epc_token_list_t *
epc_token_list_create(size_t initial_capacity)
{
    size_t byte_size = initial_capacity * sizeof(epc_parser_token_t);

    if (byte_size > EPC_TOKEN_LIST_MAX_SIZE)
    {
        byte_size = EPC_TOKEN_LIST_MAX_SIZE;
    }
    if (byte_size < 4096)
    {
        byte_size = 4096;
    }

    long const page_size = sysconf(_SC_PAGESIZE);
    size_t const total_size = EPC_TOKEN_LIST_MAX_SIZE + page_size;

    void * mem = mmap(NULL, total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (mem == MAP_FAILED)
    {
        return NULL;
    }

    if (mprotect((char *)mem + EPC_TOKEN_LIST_MAX_SIZE, page_size, PROT_NONE) != 0)
    {
        munmap(mem, total_size);
        return NULL;
    }

    epc_token_list_t * list = calloc(1, sizeof(*list));
    if (list == NULL)
    {
        munmap(mem, total_size);
        return NULL;
    }

    list->tokens = (epc_parser_token_t *)mem;
    list->count = 0;
    list->capacity = EPC_TOKEN_LIST_MAX_SIZE / sizeof(epc_parser_token_t);
    list->mmap_base = mem;
    list->mmap_size = total_size;

    return list;
}

void
epc_token_list_free(epc_token_list_t * list)
{
    if (list == NULL)
    {
        return;
    }

    if (list->mmap_base != NULL)
    {
        munmap(list->mmap_base, list->mmap_size);
    }
    free(list);
}

bool
epc_token_list_add(epc_token_list_t * list, epc_token_id_t id, epc_parser_input_view_t view)
{
    if (list == NULL)
    {
        return false;
    }

    if (list->count >= list->capacity)
    {
        return false;
    }

    list->tokens[list->count] = (epc_parser_token_t){
        .id = id,
        .view = view,
    };
    list->count++;

    return true;
}

size_t
epc_token_list_count(epc_token_list_t const * list)
{
    if (list == NULL)
    {
        return 0;
    }

    return list->count;
}

epc_parser_token_t const *
epc_token_list_data(epc_token_list_t const * list)
{
    if (list == NULL)
    {
        return NULL;
    }

    return list->tokens;
}

void
epc_token_list_detach_mmap(epc_token_list_t * list, void ** out_base, size_t * out_size)
{
    if (list == NULL)
    {
        if (out_base) *out_base = NULL;
        if (out_size) *out_size = 0;
        return;
    }

    if (out_base) *out_base = list->mmap_base;
    if (out_size) *out_size = list->mmap_size;

    list->tokens = NULL;
    list->mmap_base = NULL;
    list->mmap_size = 0;
    list->capacity = 0;
}
