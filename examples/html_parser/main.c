#include "html.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <html_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char const * filename = argv[1];

    epc_parser_list * list = epc_parser_list_create();
    if (list == NULL)
    {
        fprintf(stderr, "Failed to create parser list.\n");
        return EXIT_FAILURE;
    }

    // create_html_parser is generated from html.gdl
    // The top-level rule in html.gdl is 'Document', so the function is 'create_html_parser'
    // if the filename was html.gdl.
    // Actually, gdl_compiler uses the filename of the GDL file to name the function.
    epc_parser_t * html_parser = create_html_parser(list);
    if (html_parser == NULL)
    {
        fprintf(stderr, "Failed to create HTML parser.\n");
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }

    // Parse from file
    epc_parse_session_t session = epc_parse_file(html_parser, filename, NULL);

    if (session.result.is_error)
    {
        epc_parser_error_t * err = session.result.data.error;
        fprintf(stderr, "Parse Error: %s\n", err->message);
        fprintf(stderr, "At line %zu, col %zu\n", err->view.line_number, err->view.column_number);
        fprintf(stderr, "Expected: %s\n", err->expected);
        fprintf(stderr, "Found: %s\n", err->found);

        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }

    printf("Successfully parsed the HTML file: %s\n", filename);

    // Print the CPT
    char * cpt_str = epc_cpt_to_string(session.internal_parse_ctx, session.result.data.success);
    if (cpt_str != NULL)
    {
        printf("Concrete Parse Tree:\n%s\n", cpt_str);
        free(cpt_str);
    }

    epc_parse_session_destroy(&session);
    epc_parser_list_free(list);
    return EXIT_SUCCESS;
}
