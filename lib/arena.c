#include "arena.h"

#include <stddef.h>
#include <sys/mman.h>
#include <unistd.h>

epc_arena_t
epc_arena_create(size_t size)
{
    epc_arena_t arena = {0};

    long const page_size = sysconf(_SC_PAGESIZE);
    arena.capacity = size;
    arena.total_size = size + (size_t)page_size;

    /*
     * Allocate requested size + 1 guard page.
     * MAP_ANONYMOUS ensures no physical memory is used until written to.
     */
    void * mem = mmap(NULL, arena.total_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (mem == MAP_FAILED)
    {
        return (epc_arena_t){0};
    }

    /* Set the guard page at the very end of the requested range. */
    if (mprotect((char *)mem + size, (size_t)page_size, PROT_NONE) != 0)
    {
        munmap(mem, arena.total_size);
        return (epc_arena_t){0};
    }

    arena.base = mem;
    arena.current_offset = 0;

    return arena;
}

void
epc_arena_destroy(epc_arena_t * arena)
{
    if (arena == NULL || arena->base == NULL)
    {
        return;
    }

    munmap(arena->base, arena->total_size);
    arena->base = NULL;
    arena->total_size = 0;
    arena->current_offset = 0;
    arena->capacity = 0;
}

void *
epc_arena_alloc(epc_arena_t * arena, size_t size)
{
    if (arena == NULL || arena->base == NULL)
    {
        return NULL;
    }

    /* Round up to nearest pointer size for alignment. */
    size_t const aligned_size = (size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);

    if (arena->current_offset + aligned_size > arena->capacity)
    {
        return NULL;
    }

    void * ptr = (char *)arena->base + arena->current_offset;
    arena->current_offset += aligned_size;

    return ptr;
}
