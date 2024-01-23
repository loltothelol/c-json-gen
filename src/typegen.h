#pragma once

#include <stdio.h>
#include "types.h"

typedef struct schema_gen_ctx {
	char const *prefix;
	FILE *stream;
} SchemaGenCtx;

/* utility procedures */
void print_def_as_ctype(SchemaDef const *def, SchemaGenCtx const *ctx, int indent);

int schema_gen_struct(SchemaDef const *def, SchemaGenCtx const *ctx, int indent);
int schema_gen_typedef(SchemaDef const *def, SchemaGenCtx const *ctx, int indent);
