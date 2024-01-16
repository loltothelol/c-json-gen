#pragma once

#include <stdio.h>
#include "types.h"

typedef struct schema_gen_ctx {
	char const *prefix;
	FILE *stream;
} SchemaGenCtx;

int schema_gen_struct(SchemaDef const *def, SchemaGenCtx const *ctx, int indent);