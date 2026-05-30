#pragma once

#include "gdl_lsp_server.h"

bool gdl_tokenize_document(gdl_lsp_server_st * svr, char const * uri, char const * text);

void gdl_clear_document_cache(gdl_lsp_server_st * svr, char const * uri);

void gdl_encode_semantic_tokens(gdl_lsp_server_st * svr, char const * uri, struct json_object * id);
