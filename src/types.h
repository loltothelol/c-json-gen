#pragma once

#include <stdio.h>
#include <string.h>
#include "json.h"

#define NUM_SCHEMA_TYPES 8
#define SCHEMA_NILTYPE schema_none_type
#define X_SCHEMA_TYPES \
	SCHEMA_TYPE(object,object,schema_obj_type) \
	SCHEMA_TYPE(array,array,schema_arr_type) \
	SCHEMA_TYPE(integer,integer,schema_int_type) \
	SCHEMA_TYPE(number,double,schema_num_type) \
	SCHEMA_TYPE(string,string,schema_str_type) \
	SCHEMA_TYPE(boolean,boolean,schema_bool_type) \
	SCHEMA_TYPE(null,null,schema_null_type)

#define SCHEMA_TYPE(X,Y,Z) Z=(1U<<(json_##Y-1U)),
typedef enum schema_type {
	SCHEMA_NILTYPE, X_SCHEMA_TYPES
} SchemaType;
#undef SCHEMA_TYPE

#define SCHEMA_TYPE(X,Y,Z) if(T&Z){T&=~Z;printf(T?#X", ":#X);}
static inline void print_schema_types(SchemaType T) {
	if (!T) printf("<none>"); else { X_SCHEMA_TYPES }
}
#undef SCHEMA_TYPE

#define SCHEMA_TYPE(X,Y,Z) if(!strcmp(#X,S)){return Z;}
static inline SchemaType parse_schema_type(char const *S) {
	X_SCHEMA_TYPES return SCHEMA_NILTYPE;
}
#undef SCHEMA_TYPE
