#pragma once

#include <stddef.h>

typedef struct
{
    void * base;           /**< Base address of the memory-mapped region. */
    size_t total_size;     /**< Total size of the region, including guard page. */
    size_t current_offset; /**< Current allocation offset within the region. */
    size_t capacity;       /**< Maximum allocatable size (total_size - page_size). */
} epc_arena_t;

/**
 * @brief Creates a general-purpose arena using memory-mapped virtual memory.
 *
 * @param size The maximum allocatable size for the arena.
 * @return epc_arena_t The initialized arena structure. base will be NULL on failure.
 */
epc_arena_t epc_arena_create(size_t size);

/**
 * @brief Destroys an arena and unmaps its memory.
 *
 * @param arena Pointer to the arena to destroy.
 */
void epc_arena_destroy(epc_arena_t * arena);

/**
 * @brief Allocates memory from the arena.
 *
 * @param arena Pointer to the arena.
 * @param size The size of the memory to allocate.
 * @return void* Pointer to the allocated memory, or NULL if the arena is full.
 */
void * epc_arena_alloc(epc_arena_t * arena, size_t size);
