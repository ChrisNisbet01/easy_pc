// Generated source for token_literal_test
#include "token_literal_test.h"
#include "token_literal_test_actions.h"
#include <easy_pc/easy_pc.h>
#include <stddef.h>
#include <stdio.h>

epc_parser_t * create_token_literal_test_parser(epc_parser_list * list)
{
    if (list == NULL)
    {
        fprintf(stderr, "Error: Parser list is NULL in create_token_literal_test_parser.\n");
        return NULL;
    }

    // Forward references:

    // Rule: MyToken
    epc_parser_t * Mytoken = epc_token(list, "Mytoken", MY_CUSTOM_TOKEN);

    // Rule: AnotherToken
    epc_parser_t * Anothertoken = epc_token(list, "Anothertoken", ANOTHER_TOKEN_42);

    // Rule: EOI
    epc_parser_t * Eoi = epc_eoi(list, "eoi");

    // Rule: Program
    epc_parser_t * Program = epc_and(list, "Program", 3, Mytoken, Anothertoken, Eoi);
    epc_parser_set_ast_action(Program, EPC_AST_SEMANTIC_ACTION_PROGRAM_RULE);

    return Program;
}
