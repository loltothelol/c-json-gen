#include "parsegen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

/* ref: https://man7.org/linux/man-pages/man3/asprintf.3.html */
extern int asprintf(char **restrict strp, const char *restrict fmt, ...);

char const JSON_LOOKUP_STUB[] =
	"static json_value const *json_lookup_field(json_value const *o, char const *k)\n"
	"{\n"
	"	assert (o->type == json_object);\n"
	"	for (unsigned i = 0; i < o->u.object.length; ++i)\n"
	"		if (!strcmp(o->u.object.values[i].name, k))\n"
	"			return o->u.object.values[i].value;\n"
	"	return NULL;\n"
	"}\n";

int schema_gen_parse(SchemaDef const *def, SchemaParseGenCtx const *ctx)
{
	assert(!def->depends);
	FILE *const s = ctx->stream;

	fprintf(s, "int %s_parse_%s(", ctx->prefix, def->name);
	print_def_as_ctype(def, ctx, 0);
	fputs("*dst, json_value const *src)\n{\n", s);

	/* generate the parser, which assigns to target `dst` */
	schema_gen_parse_inl(def, "(*dst)", "src", ctx);

	fputs("\treturn 0;\n}\n", s);
	return 0;
}

int schema_gen_parse_inl(SchemaDef const *def, char const *dst_name, char const *src_name, SchemaParseGenCtx const *ctx)
{
	FILE *const s = ctx->stream;
	switch (def->type) {
		case schema_obj_type:
			for (unsigned int i = 0; i < def->u.props->length; ++i) {
				SchemaProp const prop = def->u.props->entries[i];
				if (prop.def->depends) continue;

				/* TODO: make this more robust */
				char *dst_subname;
				asprintf(&dst_subname, "%s.%s", dst_name, prop.name);
				if (!dst_subname) return -1;

				fprintf(s, "\t{json_value const *%s_json = json_lookup_field(%s, \"%s\");\n", prop.name, src_name, prop.name);
				fprintf(s, "\t%s_parse_%s(&%s, %s_json);}\n", ctx->prefix, prop.def->name, dst_subname, prop.name);
				// schema_gen_parse_inl(prop.def, dst_subname, dst_name);
				free(dst_subname);
			}
			break;
		case schema_arr_type:
			break;
		case schema_int_type:
			fprintf(s, "\tif (%s->type != json_integer) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.integer;\n", dst_name, src_name);
			break;
		case schema_num_type:
			fprintf(s, "\tif (%s->type != json_double) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.dbl;\n", dst_name, src_name);
			break;
		case schema_str_type:
			fprintf(s, "\tif (%s->type != json_string) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.string.ptr;\n", dst_name, src_name);
			break;
		case schema_bool_type:
			fprintf(s, "\tif (%s->type != json_boolean) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.boolean;\n", dst_name, src_name);
			break;
		case schema_null_type:
			fprintf(s, "\tif (%s->type != json_null) return -1;\n", src_name);
			fprintf(s, "\t%s = NULL;\n", dst_name);
			break;
		default:
			break;
			return -1;
	}

	return 0;
}
