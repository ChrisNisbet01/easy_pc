#include <assert.h>
#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h> // For AST builder APIs
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Custom AST Node for PPP Payload ---
typedef struct
{
    char * payload;
    size_t len;
} ppp_payload_ast_t;

// --- Helper for Un-stuffing ---
char *
unstuff_buffer(char const * stuffed_buf, size_t stuffed_len, size_t * unstuffed_len_out)
{
    char * unstuffed_content = malloc(stuffed_len); // Max possible size
    assert(unstuffed_content != NULL);
    size_t current_unstuffed_len = 0;

    for (size_t i = 0; i < stuffed_len; ++i)
    {
        if (stuffed_buf[i] == 0x7D && i + 1 < stuffed_len)
        {
            unstuffed_content[current_unstuffed_len++] = stuffed_buf[i + 1] ^ 0x20;
            i++;
        }
        else
        {
            unstuffed_content[current_unstuffed_len++] = stuffed_buf[i];
        }
    }

    *unstuffed_len_out = current_unstuffed_len;
    return unstuffed_content;
}

// --- Semantic Action for PPP Payload (AST building phase) ---
typedef enum
{
    PPP_AST_ACTION_UNSTUFF_PAYLOAD = 1,
    PPP_AST_ACTION_COUNT, // Always last
} ppp_ast_actions_t;

static void
ppp_unstuff_payload_action(
    epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data
)
{
    // Get the raw (stuffed) content from the CPT node
    char const * stuffed_raw_content = epc_cpt_node_get_content(node);
    size_t stuffed_raw_len = epc_cpt_node_get_len(node);

    // This node covers both payload and FCS (6 bytes total in test)
    // Payload part is the first 4 bytes of this node's content
    char const * stuffed_payload_part = stuffed_raw_content;
    size_t stuffed_payload_part_len = 4; // This value is hardcoded for this specific test case

    // Perform un-stuffing
    size_t unstuffed_len;
    char * unstuffed_buf = unstuff_buffer(stuffed_payload_part, stuffed_payload_part_len, &unstuffed_len);

    // Create our custom AST node
    ppp_payload_ast_t * payload_ast = malloc(sizeof(ppp_payload_ast_t));
    assert(payload_ast != NULL);
    payload_ast->payload = unstuffed_buf;
    payload_ast->len = unstuffed_len;

    epc_ast_push(ctx, payload_ast);
}

// --- AST Node Free Callback ---
static void
ppp_free_ast_node(void * node_ptr, void * user_data)
{
    if (node_ptr)
    {
        ppp_payload_ast_t * payload_ast = (ppp_payload_ast_t *)node_ptr;
        free(payload_ast->payload); // Free the unstuffed buffer
        free(payload_ast);          // Free the AST node itself
    }
}

static void
ppp_parser_ast_hook_registry_init(epc_ast_hook_registry_t * registry)
{
    epc_ast_hook_registry_set_free_node(registry, ppp_free_ast_node);
    epc_ast_hook_registry_set_action(registry, PPP_AST_ACTION_UNSTUFF_PAYLOAD, ppp_unstuff_payload_action);
}

int
main(int argc, char ** argv)
{
    printf("Running PPP Combinator Test\n");

    epc_parser_list * list = epc_parser_list_create();

    char const ppp_frame_buf[] = {
        0x7E, // Start Flag
        0xFF, // Address
        0x03, // Control
        0x7D,
        0x5E, // Escaped 0x7E (original 0x7E)
        0x7D,
        0x5D, // Escaped 0x7D (original 0x7D)
        0x00,
        0x00, // FCS (placeholder)
        0x7E  // End Flag
    };
    size_t ppp_frame_len = sizeof(ppp_frame_buf);

    // --- Grammar Definition ---
    epc_parser_t * stuffed_payload_and_fcs_parser = epc_count(list, "stuffed_data_and_fcs", 6, epc_any(list, "byte"));

    // Assign semantic action to the parser that matches the stuffed data
    epc_parser_set_ast_action(stuffed_payload_and_fcs_parser, PPP_AST_ACTION_UNSTUFF_PAYLOAD);

    epc_parser_t * frame_parser = epc_and(
        list,
        "ppp_frame",
        5,
        epc_byte(list, "start_flag", 0x7E),
        epc_byte(list, "address", 0xFF),
        epc_byte(list, "control", 0x03),
        stuffed_payload_and_fcs_parser, // This will match the 6 raw bytes
        epc_byte(list, "end_flag", 0x7E)
    );

    // --- Parsing Phase ---
    epc_parse_input_t input = {.type = EPC_PARSE_TYPE_BUFFER, .buffer.buf = ppp_frame_buf, .buffer.len = ppp_frame_len};

    epc_compile_result_t compile_result = epc_parse_and_build_ast(
        frame_parser, input, PPP_AST_ACTION_COUNT, ppp_parser_ast_hook_registry_init, NULL, NULL
    );

    if (!compile_result.success)
    {
        if (compile_result.parse_error_message)
        {
            fprintf(stderr, "Parse Error: %s\n", compile_result.parse_error_message);
        }
        if (compile_result.ast_error_message)
        {
            fprintf(stderr, "AST Build Error: %s\n", compile_result.ast_error_message);
        }
        epc_compile_result_cleanup(&compile_result, ppp_free_ast_node, NULL);
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }
    else
    {
        printf("Parsing and AST building successful!\n");
        printf("AST:\n");

        ppp_payload_ast_t * payload_ast = (ppp_payload_ast_t *)compile_result.ast;
        assert(payload_ast != NULL);

        // --- Validation of Unstuffed Payload ---
        assert(payload_ast->len == 2);
        assert(payload_ast->payload[0] == 0x7E);
        assert(payload_ast->payload[1] == 0x7D);
        printf(
            "Payload successfully validated: 0x%02X 0x%02X\n",
            (unsigned char)payload_ast->payload[0],
            (unsigned char)payload_ast->payload[1]
        );

        epc_compile_result_cleanup(&compile_result, ppp_free_ast_node, NULL);
        epc_parser_list_free(list);
        printf("PPP Combinator Test PASSED\n");
        return EXIT_SUCCESS;
    }

    // This part should not be reached
    return EXIT_FAILURE;
}
