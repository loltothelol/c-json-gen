#pragma once

#include "typegen.h"

typedef SchemaGenCtx SchemaParseGenCtx;
extern char const JSON_LOOKUP_STUB[];

/* generate a JSON parser for the provided type `def` */
int schema_gen_parse(SchemaDef const *def, SchemaParseGenCtx const *ctx);

/* same as above, but inline (no procedure definition) */
int schema_gen_parse_inl(SchemaDef const *def, char const *dst_name, char const *src_name, SchemaParseGenCtx const *ctx);
