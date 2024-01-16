#include "parse.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

/* merged into mainline ISO C standard as of 6/2019 */
extern char *strdup(const char *str);
extern char *strndup(const char *str, size_t size);

static char *strdup_trim(const char *str) {
	if (!str) return NULL;
	unsigned int len = strlen(str);
	while (isspace(str[len-1]) && len)
		--len;
	return strndup(str, len);
}

typedef struct str_slice {
	unsigned int len;
	char buf[];
} StrSlice;

static inline StrSlice *new_str_slice(unsigned int len) {
	size_t const siz = sizeof(StrSlice) + sizeof(char[len + 1U]);
	StrSlice *const str = malloc(siz);
	if (!str) return NULL;
	return memset(str, 0, siz);
}

typedef struct schema_ctx {
	SListInfo *defs;
	char const *base;
	unsigned base_len;
} SchemaCtx;

static SchemaDef *parse_API_schema_def(char const *name, json_value *obj, SchemaCtx const *ctx);

static json_value const *json_lookup_field(json_value const *v, char const *index)
{
	assert (v->type == json_object);
	for (unsigned i = 0; i < v->u.object.length; ++i)
		if (!strcmp(v->u.object.values[i].name, index))
			return v->u.object.values[i].value;
	return NULL;
}

static SchemaType schema_parse_types(json_value const *arr)
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

static inline SchemaDef const *lookup_schema_def(SchemaDef const *head, char const *index) {
	while (head) {
		if (!strcmp(index, head->name)) return head;
		head = (SchemaDef const *) head->head.next;
	}
	return NULL;
}

static StrSlice *create_dep_name(char const *base, unsigned int base_len, char const *field, unsigned int field_len)
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

static SchemaDef const *parse_API_schema_ref(json_value *obj, char const *fld, unsigned int fld_len, SchemaCtx const *ctx)
{
	json_value const *ref_url = json_lookup_field(obj, "$ref");
	if (ref_url) {
		if (ref_url->type != json_string) {
			printf("%s: '%.*s' has non-string $ref.\n", __func__, fld_len, fld);
			return NULL;
		}

		char const *ref_end = strrchr(ref_url->u.string.ptr, '/');
		if (ref_end) ++ref_end; /* skip leading slash */

		/* lookup the ref in the schema defs */
		SchemaDef const *match_def = lookup_schema_def((SchemaDef *) ctx->defs->head, ref_end);
		if (!match_def) {
			printf("%s: did not find a match for $ref\n", __func__);
			return NULL;
		}

		return match_def;
	}
	/* create a new dependent def */
	else {
		StrSlice *dep_name = create_dep_name(ctx->base, ctx->base_len, fld, fld_len);
		if (!dep_name) {
			printf("%s: could not create name string\n", __func__);
			return NULL;
		}

		SchemaDef *dep_def = parse_API_schema_def(dep_name->buf, obj, ctx);
		dep_def->depends = true; /* make it a dependent type-def */
		return dep_def;
	}
}

static char const *parse_opt_json_string(json_value const *val) {
	return (val && val->type == json_string) ?
		strdup_trim(val->u.string.ptr) : NULL;
}

static int parse_API_schema_prop(json_object_entry *ent, SchemaProp *prop, SchemaCtx const *ctx)
{
	char const *const nam = ent->name;
	unsigned int const nam_len = ent->name_length;

	json_value *obj = ent->value;
	if (obj->type != json_object) {
		printf ("%s: '%.*s' property is not formatted as a JSON object.\n", __func__, nam_len, nam);
		return -1;
	}

	prop->def = parse_API_schema_ref(obj, nam, nam_len, ctx);
	if (!prop->def) return -1;

	prop->name = strndup(nam, nam_len);
	printf("   + '%s' :: %s\n", prop->name, prop->def->name);

	json_value const *desc_json = json_lookup_field(obj, "description");
	prop->desc = parse_opt_json_string(desc_json);

	return 0;
}

static SchemaProps *parse_API_schema_props(json_value *obj, SchemaCtx const *ctx)
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
	props->length = len;

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

static SchemaDef *parse_API_schema_def(char const *name, json_value *obj, SchemaCtx const *ctx)
{
	assert(obj->type == json_object);

	SchemaDef *const def = new_schema_def(name, schema_none_type);
	if (!def) {
		fprintf(stderr, "%s: could not allocate schema def.\n", __func__);
		return NULL;
	}

	unsigned int const len = obj->u.object.length;
	json_object_entry *const ents = obj->u.object.values;

	for (unsigned int i = 0; i < len; ++i) {
		json_object_entry *const ent = &ents[i];
		char const *const nam = ent->name;
		unsigned int const nam_len = ent->name_length;

		printf(" - \"%.*s\"", nam_len, nam);

		if (!strncmp("type", nam, nam_len)) {
			json_value *const val = ent->value;
			if (val->type == json_string) {
				SchemaType const type = parse_schema_type(val->u.string.ptr);
				printf(" TYPE : %s\n", schema_type_name(type));
				def->type = type;
			}
			else if (val->type == json_array) {
				SchemaType const types = schema_parse_types(val);
				printf(" TYPES : ");
				print_schema_types(types);
				putchar('\n');
				def->type = types;
			}
		}
		else if (!strncmp("description", nam, nam_len)) {
			json_value *const val = ent->value;
			assert(val->type == json_string);
			char const *str = val->u.string.ptr;
			
			printf(" DESC : %s\n", str);
			def->desc = strdup_trim(str);
		}
		else if (!strncmp("properties", nam, nam_len)) {
			printf(" PROPS\n");

			json_value *const val = ent->value;
			if (val->type != json_object) {
				printf("%s: \"properties\" field is not formatted as a JSON object.\n", __func__);
				free_schema_def(def);
				goto err_free_def;
			}

			SchemaProps *props = parse_API_schema_props(val, ctx);
			if (!props) {
				printf("%s: could not parse schema properties.\n", __func__);
				goto err_free_def;
			}

			/* ensure that `def` type is object */
			if (def->type == schema_none_type) {
				def->type = schema_obj_type;
			} else if (def->type != schema_obj_type) {
				printf("%s: non-object-typed value has properties.\n", __func__);
				free_schema_def(def);
				goto err_free_def;
			}
			def->u.props = props;
		}
		else if (!strncmp("items", nam, nam_len)) {
			printf(" ITEMS\n");

			json_value *const val = ent->value;
			if (val->type != json_object) {
				printf("%s: \"items\" field is not formatted as a JSON object.\n", __func__);
				free_schema_def(def);
				goto err_free_def;
			}

			SchemaItems items = { parse_API_schema_ref(val, "[]", 2, ctx) };
			if (!items.def) {
				printf("%s: could not parse schema items.\n", __func__);
				free_schema_def(def);
				goto err_free_def;
			}

			/* ensure that `def` type is array */
			if (def->type == schema_none_type) {
				def->type = schema_arr_type;
			} else if (def->type != schema_arr_type) {
				printf("%s: non-array-typed value has items.\n", __func__);
				goto err_free_def;
			}
			def->u.items = items;
		}
		else putchar('\n');
	}

	slist_append(ctx->defs, &def->head);
	return def;

err_free_def:
	free_schema_def(def);
	return NULL;
}

static int parse_API_schemas (json_value const *schemas_obj, SListInfo *out)
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

		char *const name = strndup(ctx.base, ctx.base_len);
		if (!name) {
			/* TODO: cleanup */
			return -1;
		}

		if (!parse_API_schema_def(name, ent->value, &ctx)) {
			fprintf(stderr, "%s: could not parse schema def.\n", __func__);
			/* TODO: cleanup */
			return -1;
		}
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
