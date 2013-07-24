#include <stdlib.h>
#include "CuTest.h"
#include <string.h>
#include "streamparse.h"

void TestParsing(CuTest *tc)
{
	streamparser_t *parser;
	int res;
	const char *server;
	
	parser = streamparser_create();
	CuAssertPtrNotNull(tc, parser);
	
	res = streamparser_add(parser,
		"<?xml version='1.0'?>\r\n"
		"<stream:stream xmlns=\"jabber:client\"\r\n"
		"to=\"cloud01\"\r\n");
	CuAssertTrue(tc, res);
	res = streamparser_is_error(parser);
	CuAssertTrue(tc, !res);

	res = streamparser_add(parser,
		"version=\"1.0\"\r\n"
		"xmlns:stream=\"http://etherx.jabber.org/streams\"\r\n"
		"xml:lang=\"en\" >\r\n");
	CuAssertTrue(tc, res);
	res = streamparser_is_error(parser);
	CuAssertTrue(tc, !res);
	res = streamparser_has_server(parser);
	CuAssertTrue(tc, res);
	server = streamparser_get_server(parser);
	CuAssertStrEquals(tc, "cloud01", server);

	streamparser_delete(parser);
}

void TestParsingError(CuTest *tc)
{
	streamparser_t *parser;
	int res;
	
	parser = streamparser_create();
	CuAssertPtrNotNull(tc, parser);
	
	res = streamparser_add(parser,
		"<?xml version='1.0'?>\r\n"
		"<stream:stream xmlns=\"jabber:client\"\r\n"
		"to=\"cloud01\"\r\n"
		"/>>"
		);
	CuAssertTrue(tc, !res);
	res = streamparser_is_error(parser);
	CuAssertTrue(tc, res);
	
	streamparser_delete(parser);
}

CuSuite* StreamParseGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestParsing);
	SUITE_ADD_TEST(suite, TestParsingError);
	return suite;
}

