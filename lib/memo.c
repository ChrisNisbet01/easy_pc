#include "easy_pc_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUCKET_COUNT 16

// FNV-1a hash function
static size_t
hash_parser_and_offset(epc_parser_t * parser, size_t input_offset)
{
    size_t hash = 14695981039346656037ULL; // FNV_offset_basis for 64-bit
    unsigned char const * p_parser = (unsigned char const *)&parser;
    for (size_t i = 0; i < sizeof(parser); ++i)
    {
        hash ^= p_parser[i];
        hash *= 1099511628211ULL; // FNV_prime for 64-bit
    }
    unsigned char const * p_offset = (unsigned char const *)&input_offset;
    for (size_t i = 0; i < sizeof(input_offset); ++i)
    {
        hash ^= p_offset[i];
        hash *= 1099511628211ULL; // FNV_prime for 64-bit
    }
    return hash;
}

static void resize_table(epc_memo_table_t * table);

EASY_PC_HIDDEN
epc_parse_result_t *
epc_memo_table_get(epc_parser_ctx_t * ctx, epc_parser_t * parser, size_t input_offset)
{
    epc_memo_table_t * table = epc_parser_ctx_get_memo_table(ctx);
    if (table == NULL || table->buckets == NULL)
    {
        return NULL;
    }

    size_t hash = hash_parser_and_offset(parser, input_offset);
    size_t index = hash % table->bucket_count;

    epc_memo_entry_t * current = table->buckets[index];
    while (current != NULL)
    {
        if (current->parser == parser && current->input_offset == input_offset)
        {
            current->hit_count++;
            return &current->result;
        }
        current = current->next;
    }

    return NULL;
}

EASY_PC_HIDDEN
void
epc_memo_table_set(epc_parser_ctx_t * ctx, epc_parser_t * parser, size_t input_offset, epc_parse_result_t result)
{
    epc_memo_table_t * table = epc_parser_ctx_get_memo_table(ctx);
    if (table == NULL)
    {
        return;
    }

    if (table->buckets == NULL)
    {
        table->bucket_count = INITIAL_BUCKET_COUNT;
        table->buckets = calloc(table->bucket_count, sizeof(*table->buckets));
        if (table->buckets == NULL)
        {
            return; /* Allocation failure */
        }
    }

    // Check load factor and resize if necessary
    if (table->entry_count >= table->bucket_count * 0.75)
    {
        resize_table(table);
    }

    size_t hash = hash_parser_and_offset(parser, input_offset);
    size_t index = hash % table->bucket_count;

    // Check if it already exists (should not happen, but for safety)
    epc_memo_entry_t * current = table->buckets[index];
    while (current != NULL)
    {
        if (current->parser == parser && current->input_offset == input_offset)
        {
            epc_parser_result_cleanup(&current->result);
            current->result = epc_parse_result_copy(ctx, result);
            current->hit_count = 0;
            return;
        }
        current = current->next;
    }

    // Insert new entry
    epc_memo_entry_t * new_entry = malloc(sizeof(*new_entry));
    if (new_entry == NULL)
    {
        return; /* Allocation failure */
    }

    new_entry->parser = parser;
    new_entry->input_offset = input_offset;
    new_entry->result = epc_parse_result_copy(ctx, result);
    new_entry->hit_count = 0;
    new_entry->next = table->buckets[index];
    table->buckets[index] = new_entry;
    table->entry_count++;
}

static void
resize_table(epc_memo_table_t * table)
{
    size_t old_bucket_count = table->bucket_count;
    epc_memo_entry_t ** old_buckets = table->buckets;

    table->bucket_count *= 2;
    table->buckets = calloc(table->bucket_count, sizeof(*table->buckets));
    if (table->buckets == NULL)
    {
        // Failed to resize, keep the old table
        table->bucket_count = old_bucket_count;
        table->buckets = old_buckets;
        return;
    }

    // Rehash all entries
    for (size_t i = 0; i < old_bucket_count; ++i)
    {
        epc_memo_entry_t * current = old_buckets[i];
        while (current != NULL)
        {
            epc_memo_entry_t * next = current->next;
            size_t hash = hash_parser_and_offset(current->parser, current->input_offset);
            size_t new_index = hash % table->bucket_count;
            current->next = table->buckets[new_index];
            table->buckets[new_index] = current;
            current = next;
        }
    }

    free(old_buckets);
}

EASY_PC_HIDDEN
void
epc_memo_table_cleanup(epc_parser_ctx_t * ctx)
{
    epc_memo_table_t * table = epc_parser_ctx_get_memo_table(ctx);
    if (table == NULL || table->buckets == NULL)
    {
        return;
    }

    for (size_t i = 0; i < table->bucket_count; ++i)
    {
        epc_memo_entry_t * current = table->buckets[i];
        while (current != NULL)
        {
            epc_memo_entry_t * next = current->next;
            epc_parser_result_cleanup(&current->result);
            free(current);
            current = next;
        }
    }
    free(table->buckets);
    table->buckets = NULL;
    table->bucket_count = 0;
    table->entry_count = 0;
}

#if INCLUDE_MEMOIZATION_DEBUG
EASY_PC_HIDDEN
void
epc_memo_table_print(epc_parser_ctx_t * ctx)
{
    epc_memo_table_t * table = epc_parser_ctx_get_memo_table(ctx);
    if (table == NULL || table->entry_count == 0)
    {
        return;
    }

    printf("\n--- Memo Table Debug ---\n");
    printf("%-30s | %-10s | %-10s\n", "Parser Name", "Offset", "Hit Count");
    printf("------------------------------------------------------------\n");

    size_t max_bucket_length = 0;
    for (size_t i = 0; i < table->bucket_count; ++i)
    {
        size_t current_length = 0;
        epc_memo_entry_t * current = table->buckets[i];

        while (current != NULL)
        {
            printf(
                "%-30s | %-10zu | %-10zu\n",
                epc_parser_get_name(current->parser),
                current->input_offset,
                current->hit_count
            );
            current = current->next;
            current_length++;
        }

        if (current_length > max_bucket_length)
        {
            max_bucket_length = current_length;
        }
    }
    printf("Maximum bucket length: %zu\n", max_bucket_length);
    printf("------------------------------------------------------------\n\n");
}
#endif
