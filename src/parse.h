#pragma once

#include <stdlib.h>
#include "slist.h"
#include "types.h"

#define bool _Bool
#define true 1
#define false 0

typedef struct schema_prop SchemaProp;
typedef struct schema_props SchemaProps;
typedef struct schema_def SchemaDef;

struct schema_prop {
	char const *name;
	SchemaDef const *value;
};

struct schema_props {
	unsigned int length;
	SchemaProp entries [];
};

struct schema_def {
	SListHead head;
	char const *name;
	SchemaType type;

	/* present (i.e. non-NULL) if type is object */
	SchemaProps *props;
};

static inline SchemaDef *new_schema_def(char const *name, SchemaType type) {
	struct schema_def *def = malloc(sizeof *def);
	if (!def) return NULL;
	*def = (struct schema_def) {
		.name = name,
		.type = type
	};
	return def;
}

static inline void free_schema_def(SchemaDef *def) {
	free((char *) def->name);
	free(def->props);
	free(def);
}

int parse_API_root (json_value const *root_obj, SListInfo *out);
