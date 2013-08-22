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

void TestBuffer(CuTest *tc)
{
	buffer_t *buffer;
	unsigned char input1[] = {0x01, 0x02, 0x03};
	size_t size1 = sizeof(input1);
	unsigned char input2[] = {0x04, 0x05, 0x06, 0x07};
	size_t size2 = sizeof(input2);
	unsigned char *output;
	unsigned char check_output[] = {0x03, 0x04, 0x05, 0x06, 0x07};
	unsigned char *output2;
	size_t output_size2;
	
	buffer = buffer_create(0);
	CuAssertTrue(tc, buffer != NULL);
	CuAssertIntEquals(tc, 0, buffer_get_length(buffer));
	
	/* Append input1 to buffer */
	buffer_append(buffer, input1, size1);
	CuAssertIntEquals(tc, 3, buffer_get_length(buffer));
	
	/* Read back the data from buffer and compare with input1 */
	output = (unsigned char*) malloc(3);
	buffer_get_data(buffer, output, 3);
	CuAssertIntEquals(tc, 0, memcmp(output, input1, 3));

	/* Remove 2 bytes and verify that 1 bytes is left in the buffer */
	buffer_remove_data(buffer, 2);
	CuAssertIntEquals(tc, 1, buffer_get_length(buffer));

	/* Append input2 to buffer and verify that now buffer contains
	   1 byte from input1 and 4 bytes from input2 */
	buffer_append(buffer, input2, size2);
	CuAssertIntEquals(tc, 5, buffer_get_length(buffer));
	output = (unsigned char*) malloc(5);
	buffer_get_data(buffer, output, 5);
	CuAssertIntEquals(tc, 0, memcmp(output, check_output, 5));
	free(output);
	
	/* Get data without copying */
	buffer_peek_data(buffer, &output2, &output_size2);
	CuAssertIntEquals(tc, 5, output_size2);
	CuAssertIntEquals(tc, 0, memcmp(output2, check_output, 5));
	
	buffer_delete(buffer);
}

/* Test reading data from buffer_t into data_t (fixed buffer structure) */
void TestBuffer2(CuTest *tc)
{
	buffer_t *buffer;
	unsigned char input[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
	size_t size = sizeof(input);
	data_t data;
	byte data_buffer[4];
	
	data_init( &data, data_buffer, sizeof(data_buffer) );
	
	buffer = buffer_create(0);
	CuAssertTrue(tc, buffer != NULL);
	CuAssertIntEquals(tc, 0, buffer_get_length(buffer));
	
	/* Append input to buffer */
	buffer_append(buffer, input, size);
	CuAssertIntEquals(tc, 8, buffer_get_length(buffer));
	
	buffer_get_data2(buffer, &data, 5);
	CuAssertIntEquals( tc, 4, data_get_length(&data) );
	CuAssertTrue( tc, ( memcmp(data_get_buffer(&data), buffer->data, 4 ) == 0 ) );

	buffer_delete(buffer);
}

void TestLimitedSizeBuffer(CuTest *tc)
{
	buffer_t *buffer;
	unsigned char input1[] = {0x01, 0x02, 0x03};
	size_t size1 = sizeof(input1);
	unsigned char input2[] = {0x04, 0x05, 0x06};
	size_t size2 = sizeof(input2);
	unsigned char *output;
	unsigned char input3[] = {0x07, 0x08, 0x09};
	size_t size3 = sizeof(input3);
	int res;
	
	buffer = buffer_create(4);

	CuAssertTrue(tc, buffer != NULL);
	CuAssertIntEquals(tc, 0, buffer_get_length(buffer));
	
	/* Append input1 to buffer */
	res = buffer_append(buffer, input1, size1);
	CuAssertTrue(tc, res);
	CuAssertIntEquals(tc, 3, buffer_get_length(buffer));
	
	/* Read back the data from buffer and compare with input1 */
	output = (unsigned char*) malloc(3);
	buffer_get_data(buffer, output, 3);
	CuAssertIntEquals(tc, 0, memcmp(output, input1, 3));
	free(output);

	/* Append input2 to buffer */
	res = buffer_append(buffer, input2, size2);
	CuAssertTrue(tc, res);
	CuAssertIntEquals(tc, 4, buffer_get_length(buffer));

	/* Read back the data from buffer and compare with input2 */
	output = (unsigned char*) malloc(4);
	buffer_get_data(buffer, output, 4);
	CuAssertIntEquals(tc, 0, memcmp(output+3, input2, 1));
	free(output);

	/* Try to append input3 to full buffer */
	res = buffer_append(buffer, input3, size3);
	CuAssertTrue(tc, !res);
	CuAssertIntEquals( tc, 4, buffer_get_length(buffer) );

	buffer_delete(buffer);
}

CuSuite* UtilGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestUtilStr);
	SUITE_ADD_TEST(suite, TestUtilStrCopy);
	SUITE_ADD_TEST(suite, TestUtilStrOperations);
	SUITE_ADD_TEST(suite, TestUtilData);
	SUITE_ADD_TEST(suite, TestBuffer);
	SUITE_ADD_TEST(suite, TestBuffer2);
	SUITE_ADD_TEST(suite, TestLimitedSizeBuffer);
	return suite;
}

