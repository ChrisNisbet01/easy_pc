#pragma once

#include <easy_pc/easy_pc.h>
#include <stdint.h>

typedef uint32_t epc_token_id_t;

typedef struct
{
    epc_token_id_t id;
    epc_parser_input_view_t view;
} epc_parser_token_t;
