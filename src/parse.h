#pragma once

#include <stdlib.h>

/* misc includes */
#include "json.h"

/* local includes */
#include "slist.h"

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
	json_type type;

	/* present (i.e. non-NULL) if type is object */
	SchemaProps *props;
};

static inline SchemaDef *new_schema_def(char const *name, json_type type) {
	struct schema_def *def = malloc(sizeof *def);
	if (!def) return NULL;
	*def = (struct schema_def) {
		.name = name,
		.type = type
	};
	return def;
}

static inline void free_schema_def(SchemaDef *def) {
	/* TODO: cleanup fields */
	free((char *) def->name);
	free(def);
}

int parse_API_root (json_value const *root_obj, SListInfo *out);
