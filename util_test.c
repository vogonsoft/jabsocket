#include "CuTest.h"
#include <stdlib.h>
#include "util.h"

static void
TestUtilStr(CuTest *tc)
{
	char buffer[8];
	str_t str;
	
	str_init( &str, buffer, sizeof(buffer) );
	CuAssertIntEquals( tc, 0, str_get_length(&str) );
	CuAssertIntEquals( tc, 8, str_get_capacity(&str) );
	CuAssertStrEquals( tc, "", str_get_string(&str) );
	
	str_set_string(&str, "abc");
	CuAssertStrEquals( tc, "abc", str_get_string(&str) );
	CuAssertIntEquals( tc, 3, str_get_length(&str) );

	str_set_string(&str, "123456789012");
	CuAssertStrEquals( tc, "1234567", str_get_string(&str) );
	
	/* Setting the string with limiting the size */
	str_setn_string(&str, "123456789012", 4);
	CuAssertStrEquals( tc, "1234", str_get_string(&str) );
	str_setn_string(&str, "123456789012", 9);
	CuAssertStrEquals( tc, "1234567", str_get_string(&str) );
	str_setn_string(&str, "123456789012", 8);
	CuAssertStrEquals( tc, "1234567", str_get_string(&str) );
	str_setn_string(&str, "123456789012", 0);
	CuAssertStrEquals( tc, "", str_get_string(&str) );
}

static void
TestUtilStrCopy(CuTest *tc)
{
	char buffer1[8];
	str_t str1;
	char buffer2[16];
	str_t str2;
	
	str_init( &str1, buffer1, sizeof(buffer1) );
	str_init( &str2, buffer2, sizeof(buffer2) );

	str_set_string(&str2, "123456789012");
	str_copy_string(&str1, &str2);
	CuAssertStrEquals( tc, "1234567", str_get_string(&str1) );
	str_copy_string(&str2, &str1);
	CuAssertStrEquals( tc, "1234567", str_get_string(&str2) );
}

static void
TestUtilStrOperations(CuTest *tc)
{
	char buffer[8];
	str_t str;
	
	str_init( &str, buffer, sizeof(buffer) );

	str_trim_beginning(&str, "   123456789012");
	CuAssertStrEquals( tc, "1234567", str_get_string(&str) );
	
	str_trim_whitespace(&str, "   abcd         ");
	CuAssertStrEquals( tc, "abcd", str_get_string(&str) );

	str_trim_whitespace(&str, "   abcd");
	CuAssertStrEquals( tc, "abcd", str_get_string(&str) );

	str_trim_whitespace(&str, "abcd    ");
	CuAssertStrEquals( tc, "abcd", str_get_string(&str) );

	str_trim_whitespace(&str, "  1234567891    ");
	CuAssertStrEquals( tc, "1234567", str_get_string(&str) );

	str_trim_whitespace(&str, "      ");
	CuAssertStrEquals( tc, "", str_get_string(&str) );
	
	/* Test appending characters */
	str_set_string(&str, "12345");
	str_append_char(&str, 'a');
	CuAssertStrEquals( tc, "12345a", str_get_string(&str) );
	CuAssertIntEquals( tc, 6, str_get_length(&str) );

	str_set_string(&str, "");
	str_append_char(&str, 'a');
	CuAssertStrEquals( tc, "a", str_get_string(&str) );
	CuAssertIntEquals( tc, 1, str_get_length(&str) );

	str_set_string(&str, "1234567");
	str_append_char(&str, 'a');
	CuAssertStrEquals( tc, "1234567", str_get_string(&str) );
	CuAssertIntEquals( tc, 7, str_get_length(&str) );
}

static void
TestUtilData(CuTest *tc)
{
	byte buffer[8];
	data_t data;
	byte data_in[] = { 0x01, 0x02, 0x03 };
	byte data_in2[] = { 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9 };
	
	data_init( &data, buffer, sizeof(buffer) );

	CuAssertIntEquals( tc, 0, data_get_length(&data) );
	CuAssertIntEquals( tc, 8, data_get_capacity(&data) );
	CuAssertPtrEquals(tc, buffer, data_get_buffer(&data) );

	data_set_data( &data, data_in, sizeof(data_in) );
	CuAssertTrue( tc, ( memcmp(data_get_buffer(&data), data_in, 3 ) == 0 ) );
	CuAssertIntEquals( tc, 3, data_get_length(&data) );

	data_set_data( &data, data_in2, sizeof(data_in2) );
	CuAssertTrue( tc, ( memcmp(data_get_buffer(&data), data_in2, 8 ) == 0 ) );
	CuAssertIntEquals( tc, 8, data_get_length(&data) );
}

CuSuite* UtilGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestUtilStr);
	SUITE_ADD_TEST(suite, TestUtilStrCopy);
	SUITE_ADD_TEST(suite, TestUtilStrOperations);
	SUITE_ADD_TEST(suite, TestUtilData);
	return suite;
}

