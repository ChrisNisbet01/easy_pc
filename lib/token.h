#pragma once

#include <easy_pc/easy_pc.h>
#include <stdint.h>

typedef struct
{
    epc_token_id_t id;
    epc_parser_input_view_t view;
    uint32_t codepoint;
    uint8_t byte_len;
} epc_parser_token_t;
