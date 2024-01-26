#include "parsegen.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "str-slice.h"

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
	fputs("*dst, json_value const *src)\n{\n\t", s);

	/* generate the parser, which assigns to target `dst` */
	int rc = schema_gen_parse_inl(def, "(*dst)", "src", ctx);
	if (rc < 0) return rc;

	fputs("\n\treturn 0;\n}\n", s);
	return 0;
}

int schema_gen_parse_inl(SchemaDef const *def, char const *dst_name, char const *src_name, SchemaParseGenCtx const *ctx)
{
	FILE *const s = ctx->stream;
	switch (def->type) {
		case schema_obj_type:
			printf("%u properties\n", def->u.props->length);
			for (unsigned int i = 0; i < def->u.props->length; ++i) {
				SchemaProp const prop = def->u.props->entries[i];
				_Bool const is_last = (i == def->u.props->length - 1U);

				StrSlice *dst_subname = printf_str_slice("%s.%s", dst_name, prop.name);
				if (!dst_subname) return -1;

				if (i != 0) fputc('\t', s);
				fprintf(s, "{ json_value const *v = json_lookup_field(%s, \"%s\");\n\tif (!v) return -1;\n", src_name, prop.name);

				fputc('\t', s);
				if (prop.def->depends) {
					schema_gen_parse_inl(prop.def, dst_subname->buf, "v", ctx);
				}
				else {
					fprintf(s, "if (%s_parse_%s(&%s, v) < 0) return -1;", ctx->prefix, prop.def->name, dst_subname->buf);
					// schema_gen_parse_inl(prop.def, dst_subname, dst_name);
				}

				fputs(is_last ? " }\n" : " }\n\n", s);
				free(dst_subname);
			}
			break;
		case schema_arr_type:
			break;
		case schema_int_type:
			fprintf(s, "if (%s->type != json_integer) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.integer;", dst_name, src_name);
			break;
		case schema_num_type:
			fprintf(s, "if (%s->type != json_double) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.dbl;", dst_name, src_name);
			break;
		case schema_str_type:
			fprintf(s, "if (%s->type != json_string) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.string.ptr;", dst_name, src_name);
			break;
		case schema_bool_type:
			fprintf(s, "if (%s->type != json_boolean) return -1;\n", src_name);
			fprintf(s, "\t%s = %s->u.boolean;", dst_name, src_name);
			break;
		case schema_null_type:
			fprintf(s, "if (%s->type != json_null) return -1;\n", src_name);
			fprintf(s, "\t%s = NULL;", dst_name);
			break;
		default:
			return -1;
	}

	return 0;
}
