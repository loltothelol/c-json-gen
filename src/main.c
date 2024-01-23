/* libc includes */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* linux includes */
#include <sys/stat.h>

#include "parse.h"
#include "parsegen.h"

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
	if (argc > 2) {
		if (!strcmp ("--help", argv [1])) {
			fprintf (stderr, "Usage: c-json-gen <openapi.json>\n");
			return EXIT_FAILURE;
		}
		API_path = argv [1];
	}

	/* read the API file and parse it as JSON */
	size_t API_size;
	char *API_buf = read_file (API_path, &API_size);
	if (!API_buf) {
		fprintf (stderr, "%s: failed to read API file \"%s\"\n", __func__, API_path);
		return EXIT_FAILURE;
	}

	/* list to store the object schema defs */
	SListInfo objs;
	slist_init(&objs);

	/* traverse the parsed JSON structure */
	{
		json_value *root_obj = json_parse (API_buf, API_size);
		if (!root_obj)
			return -1;
		parse_API_root (root_obj, &objs);
		json_value_free (root_obj);
	}

	/* output the results */
	// printf("\n ===== OUTPUT =====\n");
	FILE *outfile = fopen("out/genparse.h", "w");
	if (!outfile) {
		fprintf(stderr, "%s: could not open file for writing.\n", __func__);
		return EXIT_FAILURE;
	}

	static char const PROG_STUB[] =
		"#pragma once\n"
		"#include <assert.h>\n"
		"#include <json.h>\n"
		"#include \"gentypes.h\"\n"
		"\n";
	fputs(PROG_STUB, outfile);
	fputs(JSON_LOOKUP_STUB, outfile);
	fputc('\n', outfile);

	SchemaGenCtx const ctx = { "icratkaya", outfile };

	SListHead *head = objs.head;
	while (head) {
		SchemaDef *def = (SchemaDef *) head;
		if (!def->depends) {
			schema_gen_parse(def, &ctx);
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
