#include "types.h"

#include <stdlib.h>

#define SCHEMA_TYPE(X,Y,Z) if(T&Z){T&=~Z;fputs(T?#X", ":#X,P);}
void print_schema_types(SchemaType T, FILE *P)
{
	if (!T) fputs("<none>", P); else { X_SCHEMA_TYPES }
}
#undef SCHEMA_TYPE

#define SCHEMA_TYPE(X,Y,Z) if(!strcmp(#X,S)){return Z;}
SchemaType parse_schema_type(char const *S)
{
	X_SCHEMA_TYPES return SCHEMA_NILTYPE;
}
#undef SCHEMA_TYPE

SchemaDef *new_schema_def(char const *name, SchemaType type) {
	struct schema_def *def = malloc(sizeof *def);
	if (!def) return NULL;
	*def = (struct schema_def) {
		.name = name,
		.type = type
	};
	return def;
}

void free_schema_def(SchemaDef *def) {
	free((char *) def->name);
	free(def->u.props);
	free(def);
}
