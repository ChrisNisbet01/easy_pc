#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef struct document_st
{
    char * uri;
    char * text;
} document_st;

typedef struct documents_ctx_st documents_ctx_st;

documents_ctx_st * documents_init(void);
void documents_cleanup(documents_ctx_st * docs);

document_st * documents_lookup(documents_ctx_st * docs, char const * uri);

void documents_add(documents_ctx_st * docs, document_st * doc);
void documents_remove(documents_ctx_st * docs, char const * uri);
void documents_update(documents_ctx_st * docs, char const * uri, char const * text);
