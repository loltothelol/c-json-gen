/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* linux includes */
#include <sys/stat.h>

#include "parse.h"
#include "parsegen.h"
#include "str-slice.h"

static char *read_file (char const *path, size_t *size_ptr)
{
	FILE *file = fopen (path, "r");
	if (!file)
		return NULL;

	fseek (file, 0L, SEEK_END);
	*size_ptr = ftell (file);
	fseek (file, 0L, SEEK_SET);

	char *buf = malloc (*size_ptr);
	if (!buf) {
		fprintf (stderr, "%s: could not allocate buffer.\n", __func__);
		fclose (file);
		return NULL;
	}

	long read = fread (buf, sizeof *buf, *size_ptr, file);
	if (read != *size_ptr) {
		fprintf (stderr, "%s: did not read whole file.\n", __func__);
		free (buf);
		fclose (file);
		return NULL;
	}

	return buf;
}

int main (int argc, char *argv [])
{
	char const *API_path = "openapi.json";
	char const *out_path = "out";
	if (argc > 3) {
		if (!strcmp("--help", argv[1])) {
			fprintf(stderr, "Usage: c-json-gen <input: openapi.json> <output: directory>\n");
			return EXIT_FAILURE;
		}
		API_path = argv[1];
		out_path = argv[2];
	}

	/* read the API file and parse it as JSON */
	size_t API_size;
	char *API_buf = read_file(API_path, &API_size);
	if (!API_buf) {
		fprintf (stderr, "%s: failed to read API file \"%s\"\n", __func__, API_path);
		return EXIT_FAILURE;
	}

	/* list to store the object schema defs */
	SListInfo objs;
	slist_init(&objs);

	/* traverse the parsed JSON structure */
	{
		json_value *root_obj = json_parse(API_buf, API_size);
		if (!root_obj)
			return -1;
		parse_API_root(root_obj, &objs);
		json_value_free(root_obj);
	}

	/* output the results */
	FILE *outfile;
	{
		StrSlice *outfile_path = printf_str_slice("%s/api-parse.h", out_path);
		assert(outfile_path);
		outfile = fopen(outfile_path->buf, "w");
		if (!outfile) {
			fprintf(stderr, "%s: could not open file \"%s\" for writing.\n", __func__, outfile_path->buf);
			return EXIT_FAILURE;
		}
		free(outfile_path);
	}

	static char const PROG_INCS[] =
		"#pragma once\n"
		"#include <assert.h>\n"
		"#include <string.h>\n"
		"#include <json.h>\n"
		"#include \"gentypes.h\"\n";
	fprintf(outfile, "%s\n", PROG_INCS);
	fputs(JSON_LOOKUP_STUB, outfile);
	fputc('\n', outfile);

	SchemaGenCtx const ctx = { "icratkaya", outfile };

	SListHead *head = objs.head;
	while (head) {
		SchemaDef *def = (SchemaDef *) head;
		printf ("- %s (%s)\n", def->name, schema_type_name(def->type));
		if (!def->depends) {
			if (schema_gen_parse(def, &ctx) < 0) {
				fprintf(stderr, "error while generating parser.\n");
				break;
			}
			fputc('\n', ctx.stream);
		}
		// if (def->type == schema_obj_type) {
		// 	schema_gen_struct(def, &ctx, 0);
		// 	fputc('\n', ctx.stream);
		// } else if (!def->depends) {
		// 	schema_gen_typedef(def, &ctx, 0);
		// 	fputc('\n', ctx.stream);
		// }
		head = head->next;
	}

	fclose(outfile);
	return EXIT_SUCCESS;
}
