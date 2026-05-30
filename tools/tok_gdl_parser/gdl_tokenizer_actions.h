#pragma once

#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TOKENIZER_ACTION_KEYWORD,
    TOKENIZER_ACTION_IDENTIFIER,
    TOKENIZER_ACTION_STRING_LITERAL,
    TOKENIZER_ACTION_CHAR_LITERAL,
    TOKENIZER_ACTION_NUMBER,
    TOKENIZER_ACTION_TOKEN_LITERAL,
    TOKENIZER_ACTION_CHAR_RANGE,
    TOKENIZER_ACTION_STRUCTURAL,

    TOKENIZER_ACTION_COUNT,
} gdl_tokenizer_action_t;

typedef struct
{
    epc_token_list_t * tokens;
    epc_cpt_node_t * last_match;
} gdl_tokenizer_ctx_t;

void gdl_tokenizer_hook_registry_init(epc_ast_hook_registry_t * registry, gdl_tokenizer_ctx_t * ctx);

#ifdef __cplusplus
}
#endif
