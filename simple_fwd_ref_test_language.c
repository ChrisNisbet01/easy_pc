// Generated source for simple_fwd_ref_test_language
#include "simple_fwd_ref_test_language.h"
#include "simple_fwd_ref_test_language_actions.h"
#include <easy_pc/easy_pc.h>
#include <stddef.h>
#include <stdio.h>

epc_parser_t * create_simple_fwd_ref_test_language_parser(epc_parser_list * list)
{
    if (list == NULL)
    {
        fprintf(stderr, "Error: Parser list is NULL in create_simple_fwd_ref_test_language_parser.\n");
        return NULL;
    }

    // Forward references:
    epc_parser_t * Seqchar = epc_parser_fwd_decl(list, "SeqChar");

    // Rule: Greeting
    epc_parser_t * Greeting = epc_string(list, "Greeting", "hello");

    // Rule: World
    epc_parser_t * World = epc_string(list, "World", "world");

    // Rule: CharX
    epc_parser_t * Charx = epc_char(list, "Charx", 'x');

    // Rule: CharY
    epc_parser_t * Chary = epc_char(list, "Chary", 'y');

    // Rule: CharZ
    epc_parser_t * Charz = epc_char(list, "Charz", 'z');

    // Rule: SeqGreeting
    epc_parser_t * Seqgreeting = epc_and(list, "Seqgreeting", 3, Greeting, World, Seqchar);

    // Rule: SeqChar
    epc_parser_t * Seqchar_def = epc_and(list, "Seqchar", 3, Charx, Chary, Charz);
    epc_parser_duplicate(Seqchar, Seqchar_def);

    // Rule: SimpleRule
    epc_parser_t * Simplerule = epc_or(list, "Simplerule", 2, Seqgreeting, Seqchar);

    // Rule: EOI
    epc_parser_t * Eoi = epc_eoi(list, "eoi");

    // Rule: Program
    epc_parser_t * Program = epc_and(list, "Program", 2, Simplerule, Eoi);
    epc_parser_set_ast_action(Program, EPC_AST_SEMANTIC_ACTION_PROGRAM_RULE);

    return Program;
}
