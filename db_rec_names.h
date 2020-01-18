/*
 * tabcomplete.h
 *
 *  Created on: Dec 16, 2012
 *      Author: ulrik
 */

#ifndef DB_REC_NAMES_H_
#define DB_REC_NAMES_H_

#include <stdio.h>

typedef struct dbn_string_t {
	char * data;
	char * str_end;
	size_t allocated_bytes;
} dbn_string_t;
void append_str( dbn_string_t * output,  char const * input_str, char delimeter );

typedef struct dcs_string_list_t {
	char ** strings;
	unsigned int num_strings;
	unsigned int max_num_strings;
}dcs_string_list_t;
void strlst_deep_free( dcs_string_list_t * str_list );
dcs_string_list_t * strlst_init( int num_strings );
int strlst_add_string_ptr( dcs_string_list_t * str_list, char * input_str);
int strlst_add_string_cpy( dcs_string_list_t * str_list, const char * input_str);
void strlst_sort( dcs_string_list_t * str_list, int (*compare)(const void *, const void *) );
void strlst_reverse(dcs_string_list_t *str_list);
//void strlst_sort( dcs_string_list_t * str_list, int(*compare)(void*, void*) );
void strlst_uniq( dcs_string_list_t * str_list, int (*compare)(const void *, const void *) );
void strlst_merge( dcs_string_list_t *dest, dcs_string_list_t *append);
void strlst_clear( dcs_string_list_t * str_list );
char * strlst_concatenate( dcs_string_list_t * str_list, char delimeter);
dcs_string_list_t *strlst_scan_lines( FILE * fptr, char*(*extract)(const char*) );
void strlist_print(dcs_string_list_t *ioc_list);

typedef enum {dcs_undefined, dcs_db_file, dcs_db_stdin, dcs_redirect_file, dcs_redirect_stdin, dcs_optarg}dcs_input_type;
typedef struct dcs_options_t
{
	char* search_word;
    char* word_list;
    char* database_file;
    char* redirector_file;
    dcs_input_type input_type;
} dcs_options_t;
int dcs_parse_options(int argc, char *argv[], dcs_options_t *options);
void dcs_free_options(dcs_options_t * options);

typedef struct drp_context_t {
	dcs_string_list_t * ioc_dirs;
	dcs_string_list_t * db_files;
	dcs_string_list_t * records;
	dcs_string_list_t * components;
}drp_context_t;

// scan through redirector file, looking for IOCs in the domain. Append resulting IOC dirs
// to ioc_dirs stringlist.
int drp_find_iocs( drp_context_t * cobj, FILE * redirector_file, const char * domain );
// Scan through ioc_dirs stringlist and find all database files in each iocs db dir.
// Append results to stringlist cobj->db_files.
int drp_find_db_files( drp_context_t * cobj );
dcs_string_list_t * drb_extract_record_names( dcs_string_list_t *db_files);
void drb_extract_record_names_threaded( dcs_string_list_t ** records,
										dcs_string_list_t ** components,
										dcs_string_list_t *db_files,
										char * search_word);
dcs_string_list_t * drb_filter_components(dcs_string_list_t *records);
dcs_string_list_t * drb_case_insensitive_search(dcs_string_list_t * list, const char * search);

char * dcs_extract_record_name(const char * db_line);
char * dcs_extract_ioc_dir( const char * redirector_line );
int dcs_find_iocs( char ** redirector_lines[], const char * domain, FILE *fptr);
int dcs_is_ioc_in_domain( const char * redirector_line, const char * domain);
int dcs_find_db_files(dcs_string_list_t * db_files_out, const char * ioc_dir, const char * pattern);
char * dcs_scan_lines( FILE * fptr, char*(*extract)(const char*) );
char * dcs_domain(const char * pvname);
int dcs_db_filename_strcmp(const void *p1, const void *p2);



// ==== original stuff
char * dcs_case_insensitive_search(const char * list, const char * search);
char* find_record_names();

#endif /* DB_REC_NAMES_H_ */
