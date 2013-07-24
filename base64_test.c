#include <stdlib.h>
#include "CuTest.h"
#include <string.h>
#include "base64.h"

void TestBase64Encode(CuTest *tc)
{
	char encoded_buffer[32];
	str_t encoded_str;
	
	str_init( &encoded_str, encoded_buffer, sizeof(encoded_buffer) );

	base64_encode((unsigned char *) "a", 1, &encoded_str);
	CuAssertIntEquals( tc, 4, str_get_length(&encoded_str) );
	CuAssertStrEquals( tc, "YQ==", str_get_string(&encoded_str) );

	base64_encode((unsigned char *) "ab", 2, &encoded_str);
	CuAssertIntEquals( tc, 4, str_get_length(&encoded_str) );
	CuAssertStrEquals( tc, "YWI=", str_get_string(&encoded_str) );

	base64_encode((unsigned char *) "abc", 3, &encoded_str);
	CuAssertIntEquals( tc, 4, str_get_length(&encoded_str) );
	CuAssertStrEquals( tc, "YWJj", str_get_string(&encoded_str) );

	base64_encode((unsigned char *) "abcd", 4, &encoded_str);
	CuAssertIntEquals( tc, 8, str_get_length(&encoded_str) );
	CuAssertStrEquals( tc, "YWJjZA==", str_get_string(&encoded_str) );
}

CuSuite* Base64GetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestBase64Encode);
	return suite;
}

