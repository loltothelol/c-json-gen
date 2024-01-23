#pragma once

#include <stdio.h>
#include <string.h>
#include "json.h"
#include "slist.h"

#define bool _Bool
#define true 1
#define false 0

#define NUM_SCHEMA_TYPES 8
#define SCHEMA_NILTYPE schema_none_type
#define X_SCHEMA_TYPES \
	SCHEMA_TYPE(object,  json_object,  schema_obj_type) \
	SCHEMA_TYPE(array,   json_array,   schema_arr_type) \
	SCHEMA_TYPE(integer, json_integer, schema_int_type) \
	SCHEMA_TYPE(number,  json_double,  schema_num_type) \
	SCHEMA_TYPE(string,  json_string,  schema_str_type) \
	SCHEMA_TYPE(boolean, json_boolean, schema_bool_type)\
	SCHEMA_TYPE(null,    json_null,    schema_null_type)

#define SCHEMA_TYPE(X,Y,Z) Z=(1U<<(Y-1U)),
typedef enum schema_type {
	SCHEMA_NILTYPE, X_SCHEMA_TYPES
} SchemaType;
#undef SCHEMA_TYPE

#define SCHEMA_TYPE(X,Y,Z) case Z: return #X;
static inline char const *schema_type_name(SchemaType T)
{
	switch (T) {
		X_SCHEMA_TYPES
		default: return "<none>";
	}
}
#undef SCHEMA_TYPE

void print_schema_types(SchemaType T, FILE *P);
SchemaType parse_schema_type(char const *S);

typedef struct schema_prop SchemaProp;
typedef struct schema_props SchemaProps;
typedef struct schema_items SchemaItems;
typedef struct schema_def SchemaDef;

struct schema_prop {
	char const *name, *desc;
	SchemaDef const *def;
};

struct schema_props {
	unsigned int length;
	SchemaProp entries [];
};

struct schema_items {
	SchemaDef const *def;
};

struct schema_def {
	SListHead head;
	char const *name, *desc;
	SchemaType type;
	bool depends;
	union {
		SchemaProps *props;
		SchemaItems items;
	} u;
};

SchemaDef *new_schema_def(char const *name, SchemaType type);
void free_schema_def(SchemaDef *def);
