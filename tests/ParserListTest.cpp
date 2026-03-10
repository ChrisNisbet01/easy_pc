#include "CppUTest/TestHarness.h"
#include "easy_pc_private.h" // For epc_parser_t definition (needed for testing parser allocation)
#include "parsers.h"

#include <stdlib.h> // For NULL

TEST_GROUP(ParserList)
{
    epc_parser_list * list;
    void setup()
    {
        list = epc_parser_list_create();
        CHECK_TRUE(list != NULL);
    }

    void teardown()
    {
        epc_parser_list_free(list);
        list = NULL;
    }
};

TEST(ParserList, CreateParserList_Success)
{
    CHECK_EQUAL(0, list->count);
    CHECK_TRUE(list->parsers != NULL);
}

TEST(ParserList, AddParser_Success)
{
    // Create a dummy parser (we don't need it to be functional, just a valid pointer)
    epc_parser_t * parser1 = epc_parser_fwd_decl(list, "parser1");
    CHECK_TRUE(parser1 != NULL);

    CHECK_EQUAL(1, list->count);
    CHECK_TRUE(list->parsers[0] == parser1);
}

TEST(ParserList, FreeParserList_HandlesNull)
{
    // Calling free on NULL should not crash
    // No explicit assertion needed, test passes if no crash occurs
    epc_parser_list_free(NULL);
}
