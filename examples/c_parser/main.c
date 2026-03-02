#include "c_grammar.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>

int
main()
{
    char const * input = "/* Pointers and members */\n"
                         "static int * const global_ptr = 1;\n"
                         "\n"
                         "// A simple function with math and members\n"
                         "static int some_function(int x, void * p)\n"
                         "{\n"
                         "    x = x + 10 / 2;\n"
                         "    return x * p->member.submember;\n"
                         "}\n"
                         "\n"
                         "void main(void)\n"
                         "{\n"
                         "    int ** pp = 0;\n"
                         "    global_ptr += 1;\n"
                         "\n"
                         "    return some_function(global_ptr, pp);\n"
                         "}\n";

    printf("Attempting to parse simple C input:\n%s\n", input);

    epc_parser_list * list = epc_parser_list_create();
    if (!list)
    {
        fprintf(stderr, "Failed to create parser list.\n");
        return EXIT_FAILURE;
    }

    // create_c_grammar_parser is generated from c_grammar.gdl
    epc_parser_t * c_parser = create_c_grammar_parser(list);
    if (!c_parser)
    {
        fprintf(stderr, "Failed to create C parser.\n");
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }

    // No user_ctx needed for this basic test
    epc_parse_session_t session = epc_parse_str(c_parser, input, NULL);

    if (session.result.is_error)
    {
        epc_parser_error_t * err = session.result.data.error;
        fprintf(stderr, "Parse Error: %s\n", err->message);
        fprintf(stderr, "At line %zu, col %zu\n", err->position.line + 1, err->position.col + 1);
        fprintf(stderr, "Expected: %s\n", err->expected ? err->expected : "unknown");
        fprintf(stderr, "Found: %s\n", err->found ? err->found : "unknown");

        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }

    printf("Successfully parsed the C input!\n");

    // Print the CPT
    char * cpt_str = epc_cpt_to_string(session.internal_parse_ctx, session.result.data.success);
    if (cpt_str)
    {
        printf("Concrete Parse Tree:\n%s\n", cpt_str);
        free(cpt_str);
    }

    epc_parse_session_destroy(&session);
    epc_parser_list_free(list);
    return EXIT_SUCCESS;
}
