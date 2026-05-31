#include "documents.h"

#include "utils.h"

#include <stdlib.h>
#include <string.h>

struct documents_ctx_st
{
    document_st * docs;
    size_t count;
    size_t capacity;
};

documents_ctx_st *
documents_init(void)
{
    documents_ctx_st * docs = calloc(1, sizeof(*docs));

    if (docs == NULL)
    {
        return NULL;
    }

    return docs;
}

void
documents_cleanup(documents_ctx_st * docs)
{
    if (docs == NULL)
    {
        return;
    }

    for (size_t i = 0; i < docs->count; i++)
    {
        free(docs->docs[i].uri);
        free(docs->docs[i].text);
    }
    free(docs->docs);
    free(docs);
}

document_st *
documents_lookup(documents_ctx_st * docs, char const * uri)
{
    if (docs == NULL)
    {
        return NULL;
    }

    for (size_t i = 0; i < docs->count; i++)
    {
        if (strcmp(docs->docs[i].uri, uri) == 0)
        {
            return &docs->docs[i];
        }
    }

    return NULL;
}

static void
documents_ensure_capacity(documents_ctx_st * docs)
{
    if (docs->count == docs->capacity)
    {
        docs->capacity = docs->capacity == 0 ? 4 : docs->capacity * 2;
        docs->docs = realloc(docs->docs, docs->capacity * sizeof(*docs->docs));
    }
}

void
documents_add(documents_ctx_st * docs, document_st * doc)
{
    if (docs == NULL || doc == NULL)
    {
        return;
    }

    documents_ensure_capacity(docs);
    docs->docs[docs->count].uri = strdup(doc->uri);
    docs->docs[docs->count].text = strdup(doc->text);
    docs->count++;
}

void
documents_remove(documents_ctx_st * docs, char const * uri)
{
    if (docs == NULL)
    {
        return;
    }

    for (size_t i = 0; i < docs->count; i++)
    {
        if (strcmp(docs->docs[i].uri, uri) == 0)
        {
            free(docs->docs[i].uri);
            free(docs->docs[i].text);
            docs->docs[i] = docs->docs[docs->count - 1];
            docs->count--;

            return;
        }
    }
}

void
documents_update(documents_ctx_st * docs, char const * uri, char const * text)
{
    document_st * doc = documents_lookup(docs, uri);
    if (doc != NULL)
    {
        free(doc->text);
        doc->text = strdup(text);
    }
    else
    {
        document_st new_doc;
        new_doc.uri = (char *)uri;
        new_doc.text = (char *)text;
        documents_add(docs, &new_doc);
    }
}
