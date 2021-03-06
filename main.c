/*
 * main.c
 *
 *  Created on: Dec 16, 2012
 *      Author: ulrik
 */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db_rec_names.h"

int main(int argc, char *argv[]) {
	dcs_options_t *options = calloc(1, sizeof(dcs_options_t));
	dcs_string_list_t *tmp_strlst=NULL;
	char * tmp_str = NULL;
	char * result_str = NULL;
	FILE * fptr = NULL;
	drp_context_t obj;
	obj.db_files = NULL; obj.ioc_dirs = NULL; obj.records = NULL; obj.components = NULL; obj.cache_fname = NULL;

	dcs_parse_options(argc, argv, options);

	obj.domain = dcs_domain(options->search_word);
	if (obj.domain == NULL && options->input_type != dcs_optarg) {
		fprintf(stderr, "ERROR: no valid domain found in input: \"%s\"\n", options->search_word);
		return -1;
	}

	// Parse through redirector from file or stdin
	if (options->input_type == dcs_redirect_file) {
        drp_genereate_cache_fname(&obj, options->redirector_file);
	    if (use_cache(&obj, options->redirector_file)) {
	        // read record names from cache file
//	        fprintf(stderr, "DEBUG: loading from cache\n");
	        obj.records = strlist_load_from_file(obj.cache_fname);
        } else {
	        // Open the redirector file
            fptr = fopen(options->redirector_file, "r");
            if (fptr == NULL) {
//                fprintf(stderr, "ERROR: unable to open file: %s\n", options->redirector_file );
                return -1;
            }
	    }
	} else if (options->input_type == dcs_redirect_stdin) {
		fptr = stdin;
	}

	if (fptr != NULL) {
//        fprintf(stderr, "DEBUG: Scanning DBs\n");
		// From the redirector file, find the IOC dirs
        drp_find_iocs(&obj, fptr);
		fclose(fptr); fptr = NULL;
		// Remove duplicate IOC dirs
		strlst_sort(obj.ioc_dirs, NULL);
		strlst_uniq(obj.ioc_dirs, NULL);
		drp_find_db_files(&obj);

		// Remove duplicate DB files with a few steps:
		//  1: sort the list of db files based on the DB filenames (excluding dir name)
		strlst_sort(obj.db_files, dcs_db_filename_strcmp);
		//  2: reverse the list of db files so that later versions of IOCs come first
		strlst_reverse(obj.db_files);
		//  3: remove duplicates of db files with same basename (so again excluding dir name)
		strlst_uniq(obj.db_files, dcs_db_filename_strcmp);
		obj.records = drb_extract_record_names(obj.db_files);
		//drb_extract_record_names_threaded(
		//		&obj.records, &obj.components,
		//		obj.db_files, options->search_word);

//        fprintf(stderr, "DEBUG: storing cache\n");
		dcs_cache_records(&obj);
	}

	// Now add any local records if cwd is an ioc dir with db/*.db file(s)
	// Avoiding all caching here as this is typically desired in development mode.
	dcs_string_list_t * new_records = drb_find_cwd_ioc_records();
	strlst_merge(obj.records, new_records);

	// Parse database input from file or stdin
	if (options->input_type == dcs_db_file) {
		fptr = fopen(options->database_file, "r");
		if (fptr == NULL) {
			fprintf(stderr, "ERROR: unable to open file: %s\n", options->database_file );
			return -1;
		}
	} else if (options->input_type == dcs_db_stdin) {
		fptr = stdin;
	}
	if (fptr != NULL) {
		obj.records = strlst_scan_lines(fptr, dcs_extract_record_name);
		fclose(fptr); fptr = NULL;
	}

	// Parse through wordlist input as argument to -W option
	if (options->input_type == dcs_optarg) {
		if (options->word_list) {
			result_str = dcs_case_insensitive_search(options->word_list, options->search_word);
		}
	}

	if (obj.records != NULL) {
		//puts("Records:\n");
		if (obj.components == NULL) {
			tmp_strlst = drb_filter_components(obj.records);
			obj.components = drb_case_insensitive_search(tmp_strlst, options->search_word);
			strlst_deep_free(tmp_strlst);
		}

		if (obj.components->num_strings > 1) {
			result_str = strlst_concatenate(obj.components, '\n');
		} else {
			strlst_uniq(obj.records, NULL);
			tmp_str = strlst_concatenate( obj.records, '\n');
			result_str = dcs_case_insensitive_search(tmp_str,options->search_word);
		}
		//strlst_deep_free(obj.records);
		strlst_deep_free(obj.components);
	}
	else if (obj.db_files != NULL) {
		//puts("DB files:\n");
		result_str = strlst_concatenate( obj.db_files, '\n');
		strlst_deep_free(obj.db_files);
	}
	else if (obj.ioc_dirs != NULL) {
		//puts("IOC dirs:\n");
		result_str = strlst_concatenate(obj.ioc_dirs, '\n');
		strlst_deep_free(obj.ioc_dirs);
	}

	if (result_str != NULL) puts( result_str);
	if (result_str != NULL) free (result_str);
	if (tmp_str != NULL) free (tmp_str);
	if (obj.domain != NULL) free(obj.domain);
	if (obj.cache_fname != NULL) free(obj.cache_fname);
	dcs_free_options(options);
	return 0;
}

