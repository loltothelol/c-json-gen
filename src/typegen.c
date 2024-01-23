#include "typegen.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>

static void schema_gen_shead(SchemaDef const *def, SchemaGenCtx const *ctx, int indent);
static void schema_gen_sbody(SchemaDef const *def, SchemaGenCtx const *ctx, int indent);

static inline
void findent(int indent, FILE *stream)
{
	for (int i = 0; i < indent; ++i)
		fputc('\t', stream);
}

static inline
void fblock(char const *msg, FILE *stream, int indent)
{
	findent(indent, stream);
	fputs("/*\n", stream);

	int msg_len = strlen(msg);
	while (msg_len) {
		int line = (msg_len > 60) ? 60 : msg_len;
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

void print_def_as_ctype(SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	static const char *LEN_TYPE = "unsigned";
	FILE *const s = ctx->stream;
	switch (def->type) {
		case schema_obj_type:
			if (def->depends) {
				schema_gen_shead(def, ctx, indent);
				schema_gen_sbody(def, ctx, indent + 1);
			} else {
				fprintf(s, "struct %s_%s", ctx->prefix, def->name);
			}
			break;
		case schema_arr_type:
			fprintf(s, "struct { %s len; ", LEN_TYPE);
			print_def_as_ctype(def->u.items.def, ctx, indent);
			fprintf(s, " dat[]; } *");
			break;
		case schema_int_type:
			fprintf(s, "int");
			break;
		case schema_num_type:
			fprintf(s, "double");
			break;
		case schema_str_type:
			fprintf(s, "char const *");
			break;
		case schema_bool_type:
			fprintf(s, "bool");
			break;
		default: fprintf(s, "void *");
	}
}

static void print_def_as_sfield(char const *fld, SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	FILE *const s = ctx->stream;
	if (def->depends) {
		switch (def->type) {
			case schema_obj_type:
				schema_gen_struct(def, ctx, indent);
				break;
			case schema_arr_type:
			case schema_int_type:
			case schema_num_type:
			case schema_str_type:
			case schema_bool_type:
			default:
				print_def_as_ctype(def, ctx, indent);
				fprintf(s, " %s", fld);
				break;
		}
	} else {
		fprintf(s, "%s_%s %s", ctx->prefix, def->name, fld);
	}
}

static
void schema_gen_shead(SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	FILE *const s = ctx->stream;

	/* print the desc. as a block comment */
	if (def->desc) fblock(def->desc, s, indent);

	findent(indent, s);
	if (def->depends)
		fputs("struct ", s);
	else
		fprintf(s, "struct %s_%s ", ctx->prefix, def->name);
}

static
void schema_gen_sbody(SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	FILE *const s = ctx->stream;
	fputs("{\n", s);
	for (unsigned int i = 0; i < def->u.props->length; ++i) {
		/* prepend a blank line for all fields except first */
		if (i) fputc('\n', s);

		/* add a description comment directly above the field */
		SchemaProp const prop = def->u.props->entries[i];
		if (prop.desc) {
			findent(indent, s);
			fprintf(s, "/* %s */\n", prop.desc);
		}

		findent(indent, s);
		print_def_as_sfield(prop.name, prop.def, ctx, indent);
		fputs(";\n", s);
	}
	fputc('}', s);
}

int schema_gen_struct(SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	assert(def->type == schema_obj_type);
	assert(def->u.props);
	
	FILE *const s = ctx->stream;
	schema_gen_shead(def, ctx, indent);
	schema_gen_sbody(def, ctx, indent + 1);
	if (!def->depends) fputs(";\n", s);
	
	return 0;
}

int schema_gen_typedef(SchemaDef const *def, SchemaGenCtx const *ctx, int indent)
{
	assert(def->type != schema_obj_type && !def->depends);

	FILE *const s = ctx->stream;
	fprintf(s, "/* %s */\n", def->desc);
	fprintf(s, "typedef ");
	print_def_as_ctype(def, ctx, indent);
	fprintf(s, " %s_%s;\n", ctx->prefix, def->name);

	return 0;
}
