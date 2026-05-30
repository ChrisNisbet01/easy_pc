// Generated source for long_double_test
#include "long_double_test.h"
#include "long_double_test_actions.h"
#include <easy_pc/easy_pc.h>
#include <stddef.h>
#include <stdio.h>

epc_parser_t * create_long_double_test_parser(epc_parser_list * list)
{
    if (list == NULL)
    {
        fprintf(stderr, "Error: Parser list is NULL in create_long_double_test_parser.\n");
        return NULL;
    }

    // Forward references:

    // Rule: MyLongDouble
    epc_parser_t * Mylongdouble = epc_long_double(list, "long_double");

    // Rule: EOI
    epc_parser_t * Eoi = epc_eoi(list, "eoi");

    // Rule: Program
    epc_parser_t * Program = epc_and(list, "Program", 2, Mylongdouble, Eoi);
    epc_parser_set_ast_action(Program, EPC_AST_SEMANTIC_ACTION_PROGRAM_RULE);

    return Program;
}
