#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>

int
main(void)
{
    epc_parser_list * list = epc_parser_list_create();
    if (list == NULL)
    {
        fprintf(stderr, "Failed to create parser list\n");
        return 1;
    }

    epc_parse_session_t session = {0};

    // Example 1: Manual UTF-8 character matcher
    epc_parser_t * pi_parser = epc_utf8_char(list, "pi_match", "π");
    session = epc_parse_str(pi_parser, "π", NULL);
    
    if (session.result.is_error)
    {
        fprintf(stderr, "Failed to parse pi: %s\n", session.result.data.error->message);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return 1;
    }
    printf("OK: Unicode pi (U+03C0) parser works\n");
    epc_parse_session_destroy(&session);

    // Example 2: Japanese character (by codepoint)
    epc_parser_t * day_parser = epc_utf8_char_from_codepoint(list, "day", 0x65E5);
    session = epc_parse_str(day_parser, "日", NULL);
    
    if (session.result.is_error)
    {
        fprintf(stderr, "Failed to parse day: %s\n", session.result.data.error->message);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return 1;
    }
    printf("OK: Unicode day (U+65E5) parser works\n");
    epc_parse_session_destroy(&session);

    // Example 3: String with UTF-8
    epc_parser_t * greeting_parser = epc_string(list, "greeting", "hello 世界");
    session = epc_parse_str(greeting_parser, "hello 世界", NULL);
    
    if (session.result.is_error)
    {
        fprintf(stderr, "Failed to parse 'hello 世界': %s\n", session.result.data.error->message);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return 1;
    }
    printf("OK: String with UTF-8 works\n");
    epc_parse_session_destroy(&session);

    // Example 4: ASCII character (backward compatibility)
    epc_parser_t * a_parser = epc_char(list, "a", 'a');
    session = epc_parse_str(a_parser, "a", NULL);
    
    if (session.result.is_error)
    {
        fprintf(stderr, "Failed to parse 'a': %s\n", session.result.data.error->message);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return 1;
    }
    printf("OK: ASCII char parser works\n");
    epc_parse_session_destroy(&session);

    epc_parser_list_free(list);
    printf("All UTF-8 parser tests passed!\n");
    return 0;
}
