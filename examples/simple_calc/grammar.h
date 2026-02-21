#pragma once

#include "ast_evaluator.h"

#include "easy_pc/easy_pc.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

epc_parser_t *
create_formula_grammar(
    epc_parser_list * list,
    size_t num_variables, variable_t const * variables,
    size_t num_constants, variable_t const * constants
);


