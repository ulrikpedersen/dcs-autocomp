//============================================================================
// Name        : db_rec_names.cpp
// Author      : Ulrik Kofoed Pedersen
// Version     :
// Copyright   : Do whatever you want - just don't bother me!
// Description : Hello World in C++, Ansi-style
//============================================================================
#define _GNU_SOURCE

#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <getopt.h>
#include <pthread.h>
#include <glob.h>
#include <regex.h>

 /* max length of a line of text from stdin */
#define MAX_TXT_LENGTH 600
const char * program_name;

#include "db_rec_names.h"

static const struct option dcs_longopts[] =
{
	{ "help",        no_argument,       NULL, 'h' },
	{ "words",       required_argument, NULL, 'W' },
	{ "db",          optional_argument, NULL, 'd' },
	{ "redirector",  optional_argument, NULL, 'r' },
	{ NULL, 0, NULL, 0 }
};

static void dcs_print_help(void)
{
    printf("Usage: %s COMMAND [OPTIONS] COMP\n", program_name);
    printf("Generate a list of possible completion words based on a case\n");
    printf("insensitive search of COMP in a list of words.\n");
    printf( "  COMP:    Word to search for completion.\n");
    printf("\n" \
            "    -h,        --help                 This help text\n"
    		"    -d[FILE],  --db[=FILE]            Extract EPICS record names from a database, read from FILE or stdin\n"
    		"    -r[FILE],  --redirector[=FILE]    Extract EPICS record names based on info from a redirector file, read from FILE or stdin\n"
    		"    -W[WORDS], --words=[WORDS]        Case insensitive search for COMP in the WORD list\n"
    		"\n"
            );
}

int dcs_parse_options(int argc, char *argv[], dcs_options_t *options)
{
    int status = 0;
    int optc;
    int lose = 0;
    int argindex;

    while ((optc = getopt_long (argc, argv, "hd::r::W:", dcs_longopts, NULL)) != -1)
        switch (optc)
        {
        case 'h':
        	dcs_print_help ();
            exit (EXIT_SUCCESS);
            break;
        case 'd':
        	options->input_type = dcs_db_stdin;
        	if (optarg != NULL) {
        		options->input_type = dcs_db_file;
        		options->database_file = strdup(optarg);
        	}
        	break;
        case 'r':
        	options->input_type = dcs_redirect_stdin;
        	if (optarg != NULL) {
        		options->input_type = dcs_redirect_file;
        		options->redirector_file = strdup(optarg);
        	}
        	break;
        case 'W':
        	options->input_type = dcs_optarg;
        	if (options->word_list != NULL) free(options->word_list);
        	options->word_list = calloc(strlen(optarg)+1, sizeof(char));
        	strncpy(options->word_list, optarg, strlen(optarg));
        	break;
        case '?':
            printf("Unknown option -%c. Will be ignored.\n", optopt);
            break;
        default:
            lose = 1;
            break;
        }

    if (lose)
    {
        /* Print error message and exit.  */
        if (optind < argc)
            fprintf (stderr, "%s: extra operand: %s\n",
                    program_name, argv[optind]);
        fprintf (stderr, "Try `%s --help' for more information.\n",
                program_name);
        exit (EXIT_FAILURE);
    }

    /* Get the COMP non-option argument - which may be an empty string! */
    argindex = optind;
    if (argindex < argc) {
    	options->search_word = calloc( strlen(argv[argindex]) + 1, sizeof(char));
    	strcpy(options->search_word, argv[argindex]);
    }
    else {
    	options->search_word = calloc(1, sizeof(char)); // empty string if no arg
    }

    return status;
}

void dcs_free_options(dcs_options_t * options)
{
	if (options == NULL) return;
	if (options->database_file != NULL) {
		free(options->database_file);
		options->database_file = NULL;
	}
	if (options->redirector_file != NULL) {
		free(options->redirector_file);
		options->redirector_file = NULL;
	}
	if (options->word_list != NULL) {
		free(options->word_list);
		options->word_list = NULL;
	}
	if (options->search_word != NULL) {
		free(options->search_word);
		options->search_word = NULL;
	}
	free(options);
}

int drp_find_iocs(drp_context_t *cobj, FILE *redirector_file)
{
    char line[MAX_TXT_LENGTH];
    char * ioc_dir = NULL;

    // Initialise stringlist with results if not already done
	if (cobj->ioc_dirs == NULL) cobj->ioc_dirs = strlst_init(0);

	// Loop through lines of redirector file pointer
    while ( fgets(line, MAX_TXT_LENGTH, redirector_file) != NULL ) {
    	if (dcs_is_ioc_in_domain(line, cobj->domain)) {
    		ioc_dir = dcs_extract_ioc_dir(line);
    		strlst_add_string_ptr( cobj->ioc_dirs, ioc_dir );
    	}
    }
    return 0;
}

int drp_find_db_files( drp_context_t * cobj )
{
	int i = 0;
	if (cobj->ioc_dirs == NULL) return -1;
	if (cobj->db_files == NULL) cobj->db_files = strlst_init(0);
	for (i=0; i<cobj->ioc_dirs->num_strings; i++) {
		dcs_find_db_files(cobj->db_files, cobj->ioc_dirs->strings[i], "db/*IOC*.db");
	}
	return 0;
}

void drp_genereate_cache_fname(drp_context_t * obj, const char * redirect_fname)
{
    const char * user = getenv("USER");
    const char * path = "/tmp/";
    const char * postfix = "-rec";
    size_t fname_size = strlen(path) + strlen(obj->domain) + 1 + strlen(user) + strlen(postfix) + 1;
    obj->cache_fname = calloc(fname_size, sizeof(char));
    sprintf(obj->cache_fname, "%s%s-%s%s", path, user, obj->domain, postfix);
//    fprintf(stdout, "DEBUG: filename: %s\n", obj->cache_fname);
}

boolean use_cache(drp_context_t *obj, const char * redirect_fname)
{
    int ret = 0;
    struct stat redirect_table_stat;
    struct stat cache_stat;
    ret = stat(redirect_fname, &redirect_table_stat);
    if (ret != 0){
//        fprintf(stderr, "DEBUG: unable to stat file: %s\n", redirect_fname);
//        perror("DEBUG: stat()");
        return FALSE;
    }
    ret = stat(obj->cache_fname, &cache_stat);
    if (ret != 0) {
//        fprintf(stderr, "DEBUG: unable to stat file: %s\n", obj->cache_fname);
//        perror("DEBUG: stat()");
        return FALSE;
    }
    #ifdef __linux__
        return cache_stat.st_mtim.tv_sec > redirect_table_stat.st_mtim.tv_sec;
    #elif defined __APPLE__
        return cache_stat.st_mtimespec.tv_sec > redirect_table_stat.st_mtimespec.tv_sec;
    #endif
}

dcs_string_list_t * drb_extract_record_names( dcs_string_list_t *db_files)
{
	int i;
	dcs_string_list_t * records = strlst_init(0);
	dcs_string_list_t * tmp=NULL;
	FILE * fptr = NULL;

	for (i=0;i<db_files->num_strings; i++) {
		fptr = fopen(db_files->strings[i], "r");
		if (fptr == NULL) continue;
		tmp = strlst_scan_lines(fptr, dcs_extract_record_name);
		strlst_merge(records, tmp);
		strlst_clear(tmp);
	}
	strlst_deep_free(tmp);
	return records;
}


typedef struct {
	char * db_file_name;
	char * search_word;
	dcs_string_list_t * records;
	dcs_string_list_t * components;
} thread_io_t;

void *drb_threaded_worker(void *threadio)
{
	dcs_string_list_t * tmp_strlst = NULL;
	thread_io_t *args = (thread_io_t*)threadio;
	FILE * fptr = NULL;
	//printf("threaded worker : %s\n", args->db_file_name);
	fptr = fopen(args->db_file_name, "r");
	if (fptr == NULL) return NULL;
	//printf(" threaded worker: %s\n", args->db_file_name);
	args->records = strlst_scan_lines(fptr, dcs_extract_record_name);
	fclose(fptr);
	tmp_strlst = drb_filter_components(args->records);
	args->components = drb_case_insensitive_search(tmp_strlst, args->search_word);
	strlst_deep_free(tmp_strlst);
	if (args->components->num_strings == 0) {
		strlst_deep_free(args->records); args->records = NULL;
		strlst_deep_free(args->components); args->components = NULL;
	} else {
		tmp_strlst = drb_case_insensitive_search(args->records, args->search_word);
		strlst_deep_free(args->records); args->records = NULL;
		args->records = tmp_strlst;
	}
	return NULL;
}

void drb_extract_record_names_threaded(
		dcs_string_list_t ** records,
		dcs_string_list_t ** components,
		dcs_string_list_t *db_files,
		char * search_word)
{
	pthread_t *thread = NULL;
	pthread_attr_t attr;
	thread_io_t *thread_args;
	int rc;
	long t;
	void *status;
	unsigned int num_threads = db_files->num_strings;

	*records = strlst_init(0);
	*components = strlst_init(0);

	thread = calloc(num_threads, sizeof(pthread_t));
	thread_args = calloc(num_threads, sizeof(thread_io_t));

	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for(t=0; t<num_threads; t++) {
		//printf("Main: creating thread %ld\n", t);
		thread_args[t].db_file_name = db_files->strings[t];
		thread_args[t].search_word = search_word;
		rc = pthread_create(&thread[t], &attr, drb_threaded_worker, (void *)&(thread_args[t]));
		//printf("rc=%d\n", rc);
		assert(rc == 0);
	}

	/* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);
	for(t=0; t<num_threads; t++) {
		rc = pthread_join(thread[t], &status);
		assert(rc == 0);
		//printf("Main: Join thread %ld (%ld)\n",
		//		t,(long)status);
		strlst_merge(*records, thread_args[t].records);
		strlst_merge(*components, thread_args[t].components);
	}

	strlst_sort(*components, NULL);
	strlst_uniq(*components, NULL);
	strlst_sort(*records, NULL);
	strlst_uniq(*records, NULL);
	free(thread);
	free(thread_args);
}


dcs_string_list_t * drb_filter_components(dcs_string_list_t *records)
{
	int i;
	char * colon = NULL;
	dcs_string_list_t * output = strlst_init(0);
	char *component;
	size_t component_length;

	for (i=0; i<records->num_strings; i++) {
		colon = index(records->strings[i], ':');
		if (colon != NULL) {
			component_length = colon - records->strings[i];
			component = calloc(component_length + 1, sizeof(char));
			strncpy(component, records->strings[i], component_length);
			strlst_add_string_ptr(output, component);
		}
	}
	strlst_sort(output, NULL);
	strlst_uniq(output, NULL);
	return output;
}


#define REALLOC_SIZE 256
char * dcs_case_insensitive_search(const char * list, const char * search)
{
	char * plist = (char*)list;
	char *result_word = calloc(REALLOC_SIZE-1, sizeof(char));
	char * cmp_result = NULL;
	char prev_char;
	dbn_string_t result = {NULL,NULL,0};
	//size_t result_allocation = REALLOC_SIZE;
	//char * result_list = calloc(REALLOC_SIZE, sizeof(char));

	if (list==NULL || search == NULL) return NULL;

	while (plist < list + strlen(list)) {
		cmp_result = strcasestr(plist, search);
		if (cmp_result == NULL) break;

		if (cmp_result != list) {
			prev_char = *(cmp_result-1);
			// The search result have to be a full word - so the previous
			// character has to be a whitespace.
			if (!(prev_char == '\n' || prev_char == '\r' || prev_char == ' ' || prev_char == '\t'))
			{
				// Not a full word, so we just move on to next char and search again
				//printf("Not a full word %s\n", cmp_result);
				plist = cmp_result + 1;
				continue;
			}
		}

		sscanf(cmp_result, "%s", result_word);
		plist = cmp_result + strlen(result_word) + 1;
		append_str(&result, result_word, '\n');

	}

	free(result_word);
	return result.data;
}


dcs_string_list_t * drb_case_insensitive_search(dcs_string_list_t * list, const char * search)
{
	dcs_string_list_t * result = strlst_init(0);
	int i;
	char * plist = NULL;
	char * cmp_result = NULL;

	if (list==NULL || search == NULL) return NULL;

	for (i=0;i<list->num_strings; i++) {
		plist = list->strings[i];
		cmp_result = strcasestr(plist, search);
		if (cmp_result == NULL) continue;

		strlst_add_string_cpy(result, plist);
	}

	return result;
}



#define STR_ALLOC_BLOCKS 4096
void append_str( dbn_string_t * output,  char const * input_str, char delimeter )
{
	char * tmp_result = NULL;
	size_t len_input_str;

	//printf("append_str: *output=%p input=%s max_len=%lu\n",
	//		output->data, input_str, output->allocated_bytes);

	// Initialise the result string. This is static and maintains state across function calls
	if (output->data==NULL) {
		output->data = (char*)calloc(STR_ALLOC_BLOCKS, sizeof(char));
		if (output->data == NULL) return;
		output->allocated_bytes = STR_ALLOC_BLOCKS;
		output->str_end = output->data;
	}
	//printf("append_str: *output=%p input=%s max_len=%lu\n",
	//		output->data, input_str,  output->allocated_bytes);

	// check if we have enough space allocated to append the input_str
	len_input_str = strlen(input_str);
	if ( output->allocated_bytes - (output->str_end - output->data) < len_input_str + 1) {
		//printf("reallocating more space\n");
		// if not enough space in the current allocation then allocate more space
		tmp_result = output->data;
		output->allocated_bytes += STR_ALLOC_BLOCKS;
		output->data = (char*)calloc( output->allocated_bytes+1, sizeof(char) );
		if (output->data == NULL) {
			fprintf(stderr, "ERROR: append_str failed to allocate extra memory\n");
			return;
		}
		strncpy( output->data, tmp_result, output->allocated_bytes );
		output->str_end = output->data + (output->str_end - tmp_result);
		free(tmp_result);
	}

	// append the input_str
	//printf("append_str: *output=%p/%p len_inp=%d max_res=%d\n",
	//		output->data, output->str_end, len_input_str, output->allocated_bytes);
	strncpy( output->str_end, input_str, output->allocated_bytes - (output->str_end - output->data));

	output->str_end += len_input_str;
	if (delimeter != '\0') {
		*output->str_end = delimeter;
		output->str_end++;
	}
	//return output_str;
}


char * dcs_extract_record_name(const char * db_line)
{
	char * result = NULL;
    char *substr, *tmpstr;
    size_t rec_name_len;

    // find a 'record'
	substr = strstr( db_line, "record" );
	if (substr == NULL) return NULL; // if the line does not contain the word 'record' then continue

	// Check that the line has not been commented out
	tmpstr = index(db_line, '#');
	if (tmpstr != NULL)
		if (tmpstr < substr) return NULL; // If the comment was before the word 'record' then continue

	// Find where the record name should start
	// (jump past the record type definition)
	substr = index(substr, ',');
	if (substr==NULL) return NULL;
	substr++; // get rid of the comma

	// pass by spaces, tabs and double-qoutes
	while ( *substr==' ' ||
			*substr=='\t' ||
			*substr=='"' ||
			*substr=='\0') substr++;

	// Find the end of the record name - i.e. when finding space, tab, double quote or parenthesis
	tmpstr = substr;
	while ( !( *tmpstr==' ' ||
			   *tmpstr=='\t' ||
			   *tmpstr=='"' ||
			   *tmpstr==')' ||
			   *tmpstr=='\0') ) tmpstr++;

	// catch faulty record def: no name given.
	if (tmpstr <= substr) return NULL;

	rec_name_len = tmpstr - substr; // Work out length of record name
	result = calloc( rec_name_len + 1, sizeof(char));
	if (result == NULL) return NULL;
	strncpy( result, substr, rec_name_len );
	return result;
}

char * dcs_extract_ioc_dir( const char * redirector_line )
{
	char * result = NULL;
	char * ioc_name = NULL;
	char * startup_script = NULL;
	char * ptmp=NULL;

	// Take a copy of the original string
	char * running = strdup(redirector_line);
	if (running == NULL) return NULL;

	ioc_name = strsep( &running, " "); // We dont use ioc_name for anything really
	startup_script = strsep( &running, " ");
	if (startup_script == NULL) {
		free(running);
		return NULL;
	}

	ptmp = strstr(startup_script, "bin");
	if (ptmp == NULL) {
		free(running);
		return NULL;
	}

	*ptmp = '\0'; // set end-of-string at the point where 'bin/...' starts
	result = strdup(startup_script);
	free(running);
	return result;
}

// Return number of lines found to match the domain.
// Resulting redirector lines are placed in redirector_lines. Memory will be allocated.
int dcs_is_ioc_in_domain( const char * redirector_line, const char * domain)
{
	int result = 0;
	regex_t myre;
	int err;
	char err_msg[MAX_TXT_LENGTH];
	char regexp_str[MAX_TXT_LENGTH];

	sprintf(regexp_str, "^%s-[A-Z]{2}-IOC-[0-9]{2}.*", domain);
	/* compiles the RE. If this step fails, reveals what's wrong with the RE */
	if ( (err = regcomp(&myre, regexp_str, REG_EXTENDED)) != 0 ) {
		regerror(err, &myre, err_msg, MAX_TXT_LENGTH);
		fprintf(stderr, "Error analyzing regular expression '%s': %s.\n", regexp_str, err_msg);
		return 0;
	}

	err = regexec(&myre, redirector_line, 0, NULL, 0);
	if ( err == 0 ) result=1; // match!
	else if ( err != REG_NOMATCH ) {
		/* this is when errors have been encountered */
		regerror(err, &myre, err_msg, MAX_TXT_LENGTH);
		return 0;
	}

	regfree(&myre);
	return result;
}

// Return number of files found.
// Result output is placed in db_files_out: a list of string pointers
int dcs_find_db_files(dcs_string_list_t * db_files_out, const char * ioc_dir, const char * pattern)
{
	int i, num_files=0;
	glob_t glob_result;
	size_t pattern_size =  strlen(ioc_dir) + strlen(pattern) + 2;
	char * full_pattern = calloc( pattern_size, sizeof(char));
	sprintf( full_pattern, "%s/%s", ioc_dir, pattern);
	if (glob(full_pattern, GLOB_NOSORT, NULL, &glob_result) != 0)
		return -1;

	for ( i = 0; i<glob_result.gl_pathc; i++)
	{
		strlst_add_string_cpy(db_files_out, glob_result.gl_pathv[i]);
	}
	num_files = glob_result.gl_pathc;
	globfree(&glob_result);
	free(full_pattern);
	return num_files;
}

char * dcs_scan_lines( FILE * fptr, char*(*extract)(const char*) )
{
    char text[MAX_TXT_LENGTH];
    char *result_str=NULL;
    dbn_string_t result = {NULL,NULL,0};

     /* "Read from input file pointer one line (\n terminated) at the time */
    while ( fgets(text, MAX_TXT_LENGTH, fptr) != NULL ) {
    	result_str = NULL;
    	result_str = extract(text);
    	if (result_str == NULL) continue;
    	append_str(&result, result_str, '\n');
    }
    return result.data;
}

char * dcs_domain(const char * pvname)
{
	regex_t regexp;
	const char * regexp_str = "^[A-Za-z]{2}[0-9]{2}[A-Za-z]{1}";
	int err, i;
	char err_msg[MAX_TXT_LENGTH];
	char * domain = NULL;

	if (strlen(pvname) < 5) return NULL;
	domain = (char*)calloc(6, sizeof(char));
	strncpy(domain, pvname, 5);
	if (strlen(domain) < 5) return NULL;

	// Use a regular expression to verify a valid domain
	//   First compile the regexp
	if ( (err = regcomp(&regexp, regexp_str, REG_EXTENDED)) != 0 ) {
		regerror(err, &regexp, err_msg, MAX_TXT_LENGTH);
		fprintf(stderr, "Error analysing regular expression '%s': %s.\n", regexp_str, err_msg);
		free(domain);domain=NULL;
		exit(-1);
	}

	// Perform the regexp matching
	err = regexec(&regexp, domain, 0, NULL, 0);
	if ( err != 0 ) {
		// No match. Free and NULL domain name
		free(domain); domain=NULL;
	}
	if ( err != REG_NOMATCH ) {
		/* this is when errors have been encountered */
		regerror(err, &regexp, err_msg, MAX_TXT_LENGTH);
	}

	regfree(&regexp);
	if (domain != NULL) {
		// convert to upper case
		for (i=0;i<strlen(domain);i++) {
			if (isalpha(domain[i]) != 0) {
				domain[i] = toupper(domain[i]);
			}
		}
	}
	return domain;
}



/******************** String List Interface **********************************/

/** Grow the max length of the string list by doubling it.
 */
int _strlst_grow(dcs_string_list_t * list){
	int num_new_strings = 0;
	char ** ptr_new_strings = NULL;
	if (list->max_num_strings == 0)
		num_new_strings = getpagesize();
		//num_new_strings = 4;
	else
		num_new_strings = list->max_num_strings * 2;
	//printf("Growing %d/%d to %d/%d\n",
	//		list->num_strings, list->max_num_strings,list->num_strings, num_new_strings);
	ptr_new_strings = calloc(num_new_strings, sizeof(char*));
	if (ptr_new_strings == NULL) return -1;
	memcpy(ptr_new_strings, list->strings, list->num_strings * sizeof(char*));
	free(list->strings);
	list->strings = ptr_new_strings;
	list->max_num_strings = num_new_strings;
	return num_new_strings;
}

dcs_string_list_t * strlst_init( int num_strings ) {
	dcs_string_list_t * new_list = (dcs_string_list_t*)calloc(1, sizeof(dcs_string_list_t));
	if (num_strings > 0) {
		new_list->strings = (char **)calloc(num_strings, sizeof(char*));
		if (new_list->strings != NULL) {
			new_list->max_num_strings = num_strings;
		}
	}
	return new_list;
}

void strlst_deep_free( dcs_string_list_t * str_list ) {
	int i = 0;
	if (str_list == NULL) return;

	if (str_list->strings != NULL) {
		for (i = 0; i<str_list->num_strings; i++) {
			if (str_list->strings[i] != NULL) {
				free(str_list->strings[i]);
				str_list->strings[i] = NULL;
			}
		}
		free(str_list->strings);
		str_list->strings = NULL;
	}
	free(str_list);
}

int strlst_add_string_ptr( dcs_string_list_t * str_list, char * input_str) {
	if (str_list == NULL || input_str == NULL) return -1;
	if (str_list->strings == NULL) _strlst_grow(str_list);
	if (str_list->num_strings + 1 >= str_list->max_num_strings) {
		_strlst_grow(str_list);
	}
	str_list->strings[str_list->num_strings] = input_str;
	assert(str_list->strings[str_list->num_strings] != NULL);
	str_list->num_strings++;
	return 0;
}

int strlst_add_string_cpy( dcs_string_list_t * str_list, const char * input_str) {
	char * cpy_str = NULL;
	cpy_str = strdup(input_str);
	assert(cpy_str != NULL);
	return strlst_add_string_ptr(str_list, cpy_str);
}

int dcs_db_filename_strcmp(const void *p1, const void *p2)
{
	int result = 0;
	char * p1_base = NULL;
	char * p2_base = NULL;
	p1_base = rindex(*(const char**)p1, '/');
	if (p1_base == NULL) p1_base = *(char**)p1;
	p2_base = rindex(*(const char**)p2, '/');
	if (p2_base == NULL) p2_base = *(char**)p2;

	result = strcmp(p1_base, p2_base);
	return result;
}

int dcs_cache_records(drp_context_t * obj)
{
    strlist_save_file(obj->cache_fname, obj->records);
    return 0;
}

static int _strcmp(const void *p1, const void *p2)
{
	static int counter = 0;
	counter++;
	return strcmp(* (char * const *) p1, * (char * const *) p2);
}

void strlst_sort( dcs_string_list_t * str_list, int (*compare)(const void *, const void *) ) {
	if (compare == NULL) compare = _strcmp;
	qsort( str_list->strings, str_list->num_strings, sizeof(char*), compare);
}

void strlst_reverse(dcs_string_list_t *str_list)
{
	int i = 0;
	char ** tmp_string_ptrs = NULL;
	if (str_list == NULL) return;
	if (str_list->strings == NULL) return;

	tmp_string_ptrs = calloc( str_list->max_num_strings, sizeof(char*));
	for (i=0; i<str_list->num_strings; i++) {
		tmp_string_ptrs[str_list->num_strings - i -1] = str_list->strings[i];
	}
	if (str_list->strings != NULL) {
		free(str_list->strings);
		str_list->strings = tmp_string_ptrs;
	}
}

// Sort and remove duplicate entries
void strlst_uniq( dcs_string_list_t * str_list, int(*compare)(const void*, const void*) ) {
	int i;
	char * prev_str;
	dcs_string_list_t *tmp_list = NULL;
	if (str_list == NULL) return;
	if (str_list->strings == NULL) return;

	tmp_list = strlst_init(str_list->max_num_strings);
	if (compare == NULL) compare = _strcmp;

	prev_str = "";
	//strlst_add_string_ptr( tmp_list, prev_str );
	for (i=0; i<str_list->num_strings; i++) {
		if (compare(&prev_str, &(str_list->strings[i])) != 0) {
			strlst_add_string_ptr( tmp_list, str_list->strings[i] );
			prev_str = str_list->strings[i];
		}
	}
	// Swap the internal string list pointer and free unused parts of the
	// internal structure.
	free( str_list->strings );
	str_list->strings = tmp_list->strings;
	str_list->max_num_strings = tmp_list->max_num_strings;
	str_list->num_strings = tmp_list->num_strings;
	free(tmp_list);
}

void strlst_merge( dcs_string_list_t *dest, dcs_string_list_t *append) {
	if (dest==NULL || append == NULL) return;
	if (append->strings == NULL) return;
	if (dest->strings == NULL) _strlst_grow(dest);

	if (dest->max_num_strings - dest->num_strings < append->num_strings) {
		dest->max_num_strings += append->num_strings;
		_strlst_grow(dest);
	}
	memcpy(dest->strings+dest->num_strings,
			append->strings,
			append->num_strings * sizeof(char*));
	dest->num_strings += append->num_strings;
}

void strlst_clear( dcs_string_list_t * str_list )
{
	if (str_list == NULL || str_list->strings == NULL) return;
	// Clear the string list entries
	memset(str_list->strings, 0, str_list->num_strings * sizeof(char*));
	str_list->num_strings = 0;
}

char * strlst_concatenate( dcs_string_list_t * str_list, char delimeter) {
	dbn_string_t fullstring;
	int i;
	fullstring.data = NULL;

	for (i=0; i<str_list->num_strings; i++) {
		append_str(&fullstring, str_list->strings[i], delimeter);
	}

	return fullstring.data;
}

dcs_string_list_t *strlst_scan_lines( FILE * fptr, char*(*extract)(const char*) )
{
	dcs_string_list_t * list = strlst_init(0);
    char line[MAX_TXT_LENGTH];
    char * extracted = NULL;

    while ( fgets(line, MAX_TXT_LENGTH, fptr) != NULL ) {
    	extracted = NULL;
    	if (extract != NULL)
    		extracted = extract(line);
    	else
    		extracted = strdup(line);
    	if (extracted == NULL) continue;
    	strlst_add_string_cpy(list, extracted);
    	free(extracted);
    }
	return list;
}

void strlist_print(dcs_string_list_t *ioc_list)
{
	int i=0;
	for (i=0; i<ioc_list->num_strings; i++)
	{
		fprintf(stdout, "%s\n", ioc_list->strings[i]);
	}
}

void strlist_save_file(const char *fname, const dcs_string_list_t *string_list) {
    int i;
    FILE* fptr = fopen(fname, "wb");
    for (i=0; i<string_list->num_strings; i++)
    {
        fprintf(fptr, "%s\n", string_list->strings[i]);
    }
    fclose(fptr);
}

dcs_string_list_t *strlist_load_from_file(const char* fname){
    dcs_string_list_t *result = NULL;
    int i;
    char line[256];
    size_t line_max = 256;
    FILE* fptr = fopen(fname, "r");
    result = strlst_init(65536);
    while (fgets(line, line_max, fptr) != NULL) {
        strlst_add_string_cpy(result, line);
    }
    fclose(fptr);
    return result;
}
