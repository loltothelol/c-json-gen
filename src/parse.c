#include "parse.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* merged into mainline ISO C standard as of 6/2019 */
extern char *strndup(const char *str, size_t size);

typedef struct str_slice StrSlice;
typedef struct schema_ctx SchemaCtx;

struct str_slice {
	unsigned int len;
	char buf[];
};

static inline StrSlice *new_str_slice(unsigned int len) {
	size_t const siz = sizeof(StrSlice) + sizeof(char[len + 1U]);
	StrSlice *const str = malloc(siz);
	if (!str) return NULL;
	return memset(str, 0, siz);
}

struct schema_ctx {
	SListInfo *defs;

	char const *base;
	unsigned base_len;
};

static
json_value const *json_lookup_field(json_value const *v, char const *index)
{
	assert (v->type == json_object);
	for (unsigned i = 0; i < v->u.object.length; ++i)
		if (!strcmp(v->u.object.values[i].name, index))
			return v->u.object.values[i].value;
	return NULL;
}

static
SchemaType schema_parse_types(json_value const *arr)
{
	assert(arr->type == json_array);
	SchemaType typs = schema_none_type;
	for (unsigned i = 0; i < arr->u.array.length; ++i) {
		json_value const *val = arr->u.array.values[i];
		if (val->type != json_string) {
			printf("%s: type was not formatted as a string.\n", __func__);
			return schema_none_type;
		}
		/* TODO: check validity */
		typs |= parse_schema_type(val->u.string.ptr);
	}
	return typs;
}

static inline
SchemaDef const *lookup_schema_def(SchemaDef const *head, char const *index) {
	while (head) {
		if (!strcmp(index, head->name)) return head;
		head = (SchemaDef const *) head->head.next;
	}
	return NULL;
}

static
StrSlice *create_dep_name(char const *base, unsigned int base_len, char const *field, unsigned int field_len)
{
	unsigned const name_len = base_len + 1UL + field_len;
	StrSlice *const name = new_str_slice(name_len);
	if (!name) return NULL;

	char *name_ptr = name->buf;
	char *const end_ptr = &name_ptr[name_len];

	/* append `base` */
	memcpy(name_ptr, base, base_len);
	name_ptr += base_len; assert(name_ptr < end_ptr);

	/* append '.' */
	*name_ptr = '.';
	++name_ptr; assert(name_ptr < end_ptr);

	/* append `nam` */
	memcpy(name_ptr, field, field_len);
	name_ptr += field_len; assert(name_ptr == end_ptr);

	return name;
}

static
SchemaDef const *create_dep_def(char const *fld, unsigned int fld_len, SchemaType type, SchemaCtx const *ctx)
{
	StrSlice *new_name = create_dep_name(ctx->base, ctx->base_len, fld, fld_len);
	if (!new_name) {
		printf("%s: could not create name string\n", __func__);
		return NULL;
	}

	SchemaDef *new_def = new_schema_def(new_name->buf, type);
	if (!new_def) {
		printf("%s: could not allocate structure\n", __func__);
		free(new_name);
		return NULL;
	}

	slist_append(ctx->defs, &new_def->head);
	return new_def;
}


static
int parse_API_schema_prop(json_object_entry *ent, SchemaProp *prop, SchemaCtx const *ctx)
{
	char const *const nam = ent->name;
	unsigned int const nam_len = ent->name_length;

	json_value *obj = ent->value;
	if (obj->type != json_object) {
		printf ("%s: '%.*s' property is not formatted as a JSON object.\n",
				__func__, nam_len, nam);
		return -1;
	}

	json_value const *ref_url = json_lookup_field(obj, "$ref");
	if (ref_url) {
		if (ref_url->type != json_string) {
			printf("%s: '%.*s' has non-string $ref.\n",
			   __func__, nam_len, nam);
			/* TODO: cleanup */
			return -1;
		}

		char const *ref_end = strrchr(ref_url->u.string.ptr, '/');
		if (ref_end) ++ref_end; /* skip leading slash */

		/* lookup the ref in the schema defs */
		SchemaDef const *match_def = lookup_schema_def((SchemaDef *) ctx->defs->head, ref_end);
		if (!match_def) {
			printf("%s: did not find a match for $ref\n", __func__);
			return -1;
		}

		prop->value = match_def;
	}
	/* create a new dependent def */
	else {
		json_value const *type_json = json_lookup_field(obj, "type");
		if (!type_json) {
			printf("%s: no type field provided for '%.*s'\n",
				   __func__, nam_len, nam);
			return -1;
		}

		if (type_json->type == json_string) {
			SchemaType const type = parse_schema_type(type_json->u.string.ptr);
			prop->value = create_dep_def(nam, nam_len, type, ctx);
		}
		else if (type_json->type == json_array) {
			SchemaType const types = schema_parse_types(type_json);
			prop->value = create_dep_def(nam, nam_len, types, ctx);
		}
		else {
			printf("%s: invalid type field provided for '%.*s'\n",
				   __func__, nam_len, nam);
			return -1;
		}
	}

	prop->name = strndup(nam, nam_len);
	printf("   + '%s' :: %s\n", prop->name, prop->value->name);

	return 0;
}

static
SchemaProps *parse_API_schema_props(json_value *obj, SchemaCtx const *ctx)
{
	assert(obj->type == json_object);

	unsigned int const len = obj->u.object.length;
	json_object_entry *const ents = obj->u.object.values;

	unsigned int const arr_siz = sizeof(SchemaProp[len]);
	SchemaProps *props = malloc(sizeof(SchemaProps) + arr_siz);
	if (!props) {
		printf("%s: could not allocate SchemaProps object.\n", __func__);
		return NULL;
	}

	for (unsigned int i = 0; i < len; ++i) {
		json_object_entry *const ent = &ents[i];
		int res = parse_API_schema_prop(ent, &props->entries[i], ctx);
		if (res < 0) {
			free(props);
			return NULL;
		}
	}

	return props;
}

static
int parse_API_schema_attrs(json_value *obj, struct schema_def *def, SchemaCtx const *ctx)
{
	assert(obj->type == json_object);

	unsigned int const len = obj->u.object.length;
	json_object_entry *const ents = obj->u.object.values;

	for (unsigned int i = 0; i < len; ++i) {
		json_object_entry *const ent = &ents[i];
		char const *const nam = ent->name;
		unsigned int const nam_len = ent->name_length;

		printf(" - \"%.*s\"", nam_len, nam);

		if (!strncmp("type", nam, nam_len)) {
			json_value *const val = ent->value;
			assert(val->type == json_string);
			char const *str = val->u.string.ptr;

			printf(" TYPE : %s\n", str);
			def->type = parse_schema_type(str);
		}
		else if (!strncmp("properties", nam, nam_len)) {
			printf(" PROPS\n");

			json_value *const val = ent->value;
			if (val->type != json_object) {
				printf("%s: \"properties\" field is not formatted as a JSON object.\n", __func__);
				return -1;
			}

			SchemaProps *props = parse_API_schema_props(val, ctx);
			if (!props) {
				printf("%s: could not parse schema properties.\n", __func__);
				return -1;
			}

			/* ensure that `def` type is object */
			if (def->type == schema_none_type) {
				def->type = schema_obj_type;
			} else if (def->type != schema_obj_type) {
				printf("%s: non-object-typed value has properties.\n", __func__);
				return -1;
			}
		}
		else {
			putchar('\n');
		}
	}

	return 0;
}

static
struct schema_def *parse_API_schema(json_value *obj, SchemaCtx const *ctx)
{
	char *const name = strndup(ctx->base, ctx->base_len);
	if (!name) return NULL;

	SchemaDef *const def = new_schema_def(name, schema_none_type);
	if (!def) {
		fprintf(stderr, "%s: could not allocate schema def.\n", __func__);
		free(name);
		return NULL;
	}

	int res = parse_API_schema_attrs(obj, def, ctx);
	if (res < 0) {
		fprintf(stderr, "%s: could not parse schema attributes.\n", __func__);
		free_schema_def(def);
		return NULL;
	}

	return def;
}

static
int parse_API_schemas (json_value const *schemas_obj, SListInfo *out)
{
	assert (schemas_obj->type == json_object);

	unsigned int len = schemas_obj->u.object.length;
	json_object_entry *vals = schemas_obj->u.object.values;

	for (unsigned int i = 0; i < len; ++i) {
		json_object_entry *ent = &vals[i];
		printf("schema [%d]: \"%.*s\"\n", i, ent->name_length, ent->name);

		SchemaCtx const ctx = {
			.defs = out,
			.base = ent->name,
			.base_len = ent->name_length
		};

		SchemaDef *def = parse_API_schema(ent->value, &ctx);
		if (!def) return -1;
		slist_append(out, &def->head);
	}

	return 0;
}

int parse_API_root (json_value const *root_obj, SListInfo *out)
{
	json_value const *components_obj = json_lookup_field (root_obj, "components");
	if (!components_obj) {
		printf ("%s: could not locate 'components' JSON field.\n", __func__);
		return -1;
	}

	json_value const *schemas_obj = json_lookup_field (components_obj, "schemas");
	if (!schemas_obj) {
		printf ("%s: could not locate 'schemas' JSON field.\n", __func__);
		return -1;
	}

	return parse_API_schemas (schemas_obj, out);
}
