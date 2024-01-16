#include "gen.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

static inline
void findent(int indent, FILE *stream)
{
	for (int i = 0; i < indent; ++i)
		fputc('\t', stream);
}

static inline
void fblock(char const *msg, FILE *stream)
{
	int msg_len = strlen(msg);
	fputs("/*\n", stream);
	while (msg_len) {
		int line = (msg_len > 40) ? 40 : msg_len;	
		/* search for a break (whitespace) */
		while (!isspace(msg[line]) && line < msg_len)
			++line;	
		fprintf(stream, " *  %.*s\n", line, msg);
		
		/* skip over any remaining whitespace */
		while (isspace(msg[line]) && line < msg_len)
			++line;
			
		msg += line;
		msg_len -= line;
	}
	fputs(" */\n", stream);
}

static void print_def_as_ctype(char const *fld, SchemaDef const *def, FILE *s, SchemaGenCtx const *ctx, int indent)
{
	if (def->depends) {
		switch (def->type) {
			case schema_obj_type:
				schema_gen_struct(def, ctx, indent);
				break;
			case schema_arr_type:
				print_def_as_ctype("", def->u.items.def, s, ctx, indent + 1);
				fprintf(s, "%s[]", fld);
				break;
			case schema_int_type:
				fprintf(s, "int %s", fld);
				break;
			case schema_num_type:
				fprintf(s, "double %s", fld);
				break;
			case schema_str_type:
				fprintf(s, "char const *%s", fld);
				break;
			case schema_bool_type:
				fprintf(s, "bool %s", fld);
				break;
			default: fprintf(s, "void *%s", fld);
		}
	} else {
		fprintf(s, "%s_%s %s", ctx->prefix, def->name, fld);
	}
}

int schema_gen_struct(SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	assert(def->type == schema_obj_type);
	assert(def->u.props);
	
	FILE *const s = ctx->stream;
	
	findent(indent, s);
	if (def->depends)
		fprintf(s, "struct {\n");
	else {
		/* print the desc. as a block comment */
		if (def->desc) fblock(def->desc, s);
		fprintf(s, "struct %s_%s {\n", ctx->prefix, def->name);
	}
	
	++indent;
	for (unsigned int i = 0; i < def->u.props->length; ++i) {
		if (i) putchar('\n');
		SchemaProp const prop = def->u.props->entries[i];
		if (prop.desc) {
			findent(indent, s);
			fprintf(s, "/* %s */\n", prop.desc);
		}
		findent(indent, s);
		print_def_as_ctype(prop.name, prop.def, s, ctx, indent);
		puts(";");
	}
	--indent;
	
	findent(indent, s);
	if (def->depends) fputc('}', s);
	else fputs("};\n", s);
	
	return 0;
}
