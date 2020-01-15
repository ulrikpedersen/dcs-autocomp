/*
 * test_db_rec_names.cpp
 *
 *  Created on: Dec 16, 2012
 *      Author: ulrik
 */


#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DbRecNamesTestModule
#include <boost/test/unit_test.hpp>

#include <stdio.h>
#include <stdlib.h>

#include <string>

extern "C" {
#include "db_rec_names.h"
}

using namespace std;

struct DbRecNamesFixture {
	dbn_string_t *pstr;
	FILE * test_small_db;
	FILE * test_redirect_small;
	char * data_str;
	dcs_string_list_t *string_list;

	DbRecNamesFixture() {
		BOOST_TEST_MESSAGE("Setup DbRecNamesFixture");
		pstr = (dbn_string_t*)calloc(1, sizeof(dbn_string_t));
		test_small_db = fopen("testdata/test_small.db", "r");
		test_redirect_small = fopen("testdata/redirect_small", "r");
		data_str = NULL;
		string_list = strlst_init(10);
	}
	~DbRecNamesFixture(){
		if (pstr != NULL) {
			if (pstr->data != NULL) free(pstr->data);
			free(pstr);
		}
		if (test_small_db != NULL) fclose(test_small_db);
		if (test_redirect_small != NULL) fclose(test_redirect_small);
		if (data_str != NULL) free(data_str);
		strlst_deep_free(string_list);
	}
};

BOOST_FIXTURE_TEST_SUITE( test_str_methods, DbRecNamesFixture )

BOOST_AUTO_TEST_CASE( test_append )
{
	BOOST_REQUIRE_NO_THROW( append_str(pstr, "one", '\0'));
	BOOST_REQUIRE_NO_THROW( append_str(pstr, "two", '\0'));
	BOOST_REQUIRE_EQUAL( pstr->data, "onetwo" );
	BOOST_REQUIRE_EQUAL( pstr->str_end, (pstr->data + 6) ); // to fail
}

BOOST_AUTO_TEST_CASE( test_extract_recname )
{
	BOOST_REQUIRE( dcs_extract_record_name("record(mbbi, BLAH-MO-DEV-02:WHAT) {\n") != NULL);
	BOOST_REQUIRE_EQUAL( dcs_extract_record_name( "record(mbbi, BLAH-MO-DEV-02:WHAT) {\n"), "BLAH-MO-DEV-02:WHAT" );

	BOOST_REQUIRE( dcs_extract_record_name("record(mbbi, BLAH-MO-DEV-02:WHAT) {") != NULL);
	BOOST_REQUIRE_EQUAL( dcs_extract_record_name( "record(mbbi, BLAH-MO-DEV-02:WHAT) {"), "BLAH-MO-DEV-02:WHAT" );
}

BOOST_AUTO_TEST_CASE( test_extract_recname_quotes )
{
	BOOST_REQUIRE( dcs_extract_record_name("record(mbbi, \"BLAH-MO-DEV-02:WHAT\") {\n") != NULL);
	BOOST_REQUIRE_EQUAL( dcs_extract_record_name( "record(mbbi, \"BLAH-MO-DEV-02:WHAT\") {\n"), "BLAH-MO-DEV-02:WHAT" );
}

BOOST_AUTO_TEST_CASE( test_extract_ioc_dir )
{
	BOOST_REQUIRE( dcs_extract_ioc_dir("BL12I-MO-IOC-02 /blah/bluh/ioc/bin/bleh/stbluh.boot\n") != NULL);
	BOOST_REQUIRE_EQUAL( dcs_extract_ioc_dir( "BL12I-MO-IOC-02 /blah/bluh/ioc/bin/bleh/stbluh.boot\n"), "/blah/bluh/ioc/" );
}

BOOST_AUTO_TEST_CASE( test_find_db_files )
{
	int nfiles=0;
	BOOST_REQUIRE_GT( nfiles=dcs_find_db_files(string_list, "testdata", "*.db"), 0);
	BOOST_REQUIRE_EQUAL( nfiles, 3 );
	BOOST_MESSAGE( string_list->strings[0] );
	BOOST_MESSAGE( string_list->strings[1] );
	BOOST_MESSAGE( string_list->strings[2] );
	char * files;
	BOOST_REQUIRE_NO_THROW( files = strlst_concatenate(string_list, ' '));
	BOOST_MESSAGE( files );
	BOOST_REQUIRE_NO_THROW( strlst_sort(string_list) );
	BOOST_REQUIRE_NO_THROW( files = strlst_concatenate(string_list, ' '));
	BOOST_MESSAGE( files );
	free(files);
}

BOOST_AUTO_TEST_CASE( test_scan_db_file )
{
	BOOST_REQUIRE_NE( test_small_db, (FILE*)NULL );
	BOOST_REQUIRE_NO_THROW( data_str = dcs_scan_lines(test_small_db, &dcs_extract_record_name ) );
	BOOST_REQUIRE_NE( data_str, (char*)NULL );
	BOOST_REQUIRE_NE(strstr(data_str, "BL12I-DI-PHDGN-01:INFO:NFLOW"), (char*)NULL);
	BOOST_REQUIRE_NE(strstr(data_str, "BL12I-MO-IOC-01:SCAN:CHKD02"), (char*)NULL);
	BOOST_REQUIRE_NE(strstr(data_str, "BL12I-OP-DCM-01:XTAL1:SCAN:FANT1"), (char*)NULL);
}

BOOST_AUTO_TEST_CASE( test_scan_redirect_file )
{
	BOOST_REQUIRE_NE( test_redirect_small, (FILE*)NULL );
	BOOST_REQUIRE_NO_THROW( data_str = dcs_scan_lines(test_redirect_small, &dcs_extract_ioc_dir ) );
	BOOST_REQUIRE_NE( data_str, (char*)NULL );
	BOOST_REQUIRE_NE(strstr(data_str, "blah/blubh/blih/"), (char*)NULL);
	BOOST_REQUIRE_NE(strstr(data_str, "/dls_sw/blib/blop/"), (char*)NULL);
}


BOOST_AUTO_TEST_SUITE_END() // DbRecNamesFixture


struct StrListFixture {
	dbn_string_t *pstr;
	dcs_string_list_t *string_list;
	char * fullstring;

	StrListFixture() {
		BOOST_TEST_MESSAGE("Setup StrListFixture");
		fullstring = NULL;
		pstr = (dbn_string_t*)calloc(1, sizeof(dbn_string_t));
		string_list = strlst_init(10);
	}
	~StrListFixture(){
		if (pstr != NULL) {
			if (pstr->data != NULL) free(pstr->data);
			free(pstr);
		}
		strlst_deep_free(string_list);
		if (fullstring != NULL) free(fullstring);
	}
};

BOOST_FIXTURE_TEST_SUITE( test_strlist_methods, StrListFixture )

BOOST_AUTO_TEST_CASE( test_uniq )
{
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "one"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "two"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "three"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "one"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "four"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "five"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "six"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "one zero"), 0 );
	BOOST_REQUIRE_EQUAL( strlst_add_string_cpy(string_list, "one two"), 0 );
	BOOST_REQUIRE_NE( fullstring = strlst_concatenate( string_list, '\n'), (char*)NULL );
	BOOST_MESSAGE( fullstring );
	BOOST_REQUIRE_NO_THROW( strlst_uniq(string_list, NULL) );
	//BOOST_REQUIRE_NO_THROW( strlst_sort(string_list) );
	free(fullstring);
	BOOST_REQUIRE_NE( fullstring = strlst_concatenate( string_list, '\n'), (char*)NULL );
	BOOST_MESSAGE( fullstring );
}

BOOST_AUTO_TEST_SUITE_END() // test_strlist_methods StrListFixture
