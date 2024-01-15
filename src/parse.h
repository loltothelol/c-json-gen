#pragma once

#include <stdlib.h>
#include <stdio.h>

#define bool _Bool
#define true 1
#define false 0

/* misc includes */
#include "json.h"

/* local includes */
#include "slist.h"

typedef struct schema_prop SchemaProp;
typedef struct schema_props SchemaProps;
typedef struct schema_def SchemaDef;

#define NUM_SCHEMA_TYPES 8

typedef enum schema_type {
	schema_none = 0x0,
	schema_string = 0x1,
	schema_number = 0x2,
	schema_integer = 0x4,
	schema_boolean = 0x8,
	schema_array = 0x10,
	schema_object = 0x20,
	schema_nullable = 0x40
} SchemaType;

static char const *SCHEMA_TYPE_NAME[NUM_SCHEMA_TYPES] = {
	"none", "string", "number", "integer",
	"boolean", "array", "object", "nullable"
};

static inline void print_schema_types(SchemaType types) {
	bool any = false;
	for (int i = 0; i < NUM_SCHEMA_TYPES; ++i) {
		bool const set = (types >> i) & 1;
		if (set) {
			if (any)
				fputs(" | ", stdout);
			else
				any = true;
			fputs(SCHEMA_TYPE_NAME[i + 1], stdout);
		}
	}
	if (!any)
		fputs(SCHEMA_TYPE_NAME[0], stdout);
}

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
