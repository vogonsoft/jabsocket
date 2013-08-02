#include <stdlib.h>
#include "CuTest.h"
#include "rqparser.h"
#include <string.h>

void TestCreateParser(CuTest *tc)
{
	request_t *h = rq_create();
	
	CuAssertPtrNotNull(tc, h);
	rq_delete(h);
}

void TestParseSimpleRequest(CuTest *tc)
{
	request_t *h = rq_create();
	
	CuAssert(tc, "Parsing cannot be in the state \"done\"", !rq_done(h));
		
	rq_add_line(h, "GET /something HTTP/1.1");
	CuAssertTrue(tc, !rq_done(h));
	rq_add_line(h, "Host: example.com");
	CuAssertTrue(tc, !rq_done(h));
	rq_add_line(h, "");
	CuAssertTrue(tc, rq_done(h));
	
	CuAssert(tc, "Parsing failed", !rq_is_error(h));
	CuAssertStrEquals_Msg(tc, "Host was not parsed properly", "example.com", rq_get_host(h));

	rq_delete(h);
}

void TestParseRequestWithContinuation(CuTest *tc)
{
	request_t *h = rq_create();
	
	CuAssert(tc, "Parsing cannot be in the state \"done\"", !rq_done(h));
		
	rq_add_line(h, "GET /something HTTP/1.1");
	rq_add_line(h, "Host: example");
	rq_add_line(h, " .com");
	rq_add_line(h, "");
	
	CuAssert(tc, "Parsing failed", !rq_is_error(h));
	CuAssertStrEquals_Msg(tc, "Host was not parsed properly", "example.com", rq_get_host(h));

	rq_delete(h);
}

void TestParseRequiredHeaders(CuTest *tc)
{
	request_t *h = rq_create();
	
	CuAssert(tc, "Parsing cannot be in the state \"done\"", !rq_done(h));

	/* This example is from Wikipedia
	   (http://en.wikipedia.org/wiki/WebSocket#WebSocket_protocol_handshake) */
	rq_add_line(h, "GET /mychat HTTP/1.1");
	rq_add_line(h, "Host: server.example.com");
	rq_add_line(h, "Upgrade: websocket");
	rq_add_line(h, "Connection: Upgrade");
	rq_add_line(h, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(h, "Sec-WebSocket-Protocol: chat");
	rq_add_line(h, "Sec-WebSocket-Version: 13");
	rq_add_line(h, "Origin: http://example.com");
	CuAssertTrue(tc, !rq_done(h));
	rq_add_line(h, "");
	
	CuAssert(tc, "Parsing failed", !rq_is_error(h));
	CuAssertStrEquals(tc, "server.example.com", rq_get_host(h));
	CuAssertStrEquals(tc, "websocket", rq_get_upgrade(h));
	CuAssertStrEquals(tc, "Upgrade", rq_get_connection(h));
	CuAssertStrEquals(tc, "x3JJHMbDL1EzLkh9GBhXDw==", rq_get_websocket_key(h));
	CuAssertStrEquals(tc, "chat", rq_get_websocket_protocol(h));
	CuAssertStrEquals(tc, "13", rq_get_websocket_version(h));
	CuAssertStrEquals(tc, "http://example.com", rq_get_origin(h));

	CuAssertTrue(tc, rq_done(h));
	CuAssertTrue(tc, !rq_is_error(h));

	rq_delete(h);
}

void TestMultipleProtocols(CuTest *tc)
{
	request_t *h = rq_create();
	
	CuAssert(tc, "Parsing cannot be in the state \"done\"", !rq_done(h));

	/* This example is from Wikipedia
	   (http://en.wikipedia.org/wiki/WebSocket#WebSocket_protocol_handshake) */
	rq_add_line(h, "GET /mychat HTTP/1.1");
	rq_add_line(h, "Host: server.example.com");
	rq_add_line(h, "Upgrade: websocket");
	rq_add_line(h, "Connection: Upgrade");
	rq_add_line(h, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(h, "Sec-WebSocket-Protocol: xmpp, smtp");
	rq_add_line(h, "Sec-WebSocket-Version: 13");
	rq_add_line(h, "Origin: http://example.com");
	CuAssertTrue(tc, !rq_done(h));
	rq_add_line(h, "");
	
	// CuAssertStrEquals(tc, "xmpp, smtp", rq_get_websocket_protocol(h));
	CuAssertIntEquals(tc, 2, rq_get_protocol_count(h));
	CuAssertStrEquals(tc, "smtp", rq_get_protocol(h, 0));
	CuAssertStrEquals(tc, "xmpp", rq_get_protocol(h, 1));
	
	CuAssertTrue(tc, rq_protocols_contains(h, "xmpp"));
	CuAssertTrue(tc, rq_protocols_contains(h, "smtp"));
	CuAssertTrue(tc, !rq_protocols_contains(h, "unknown"));

	CuAssertTrue(tc, rq_done(h));
	CuAssertTrue(tc, !rq_is_error(h));

	rq_delete(h);
}

void TestTrim(CuTest *tc)
{
	char *str = "  value";
	char *str2 = strdup(str);
	char *str3 = trim_beginning(str2);
	CuAssertStrEquals_Msg(tc, "trim_beginning did not generate correct result", "value", str3);
	free(str3); /* note that trim_beginning took care of freeing str3 */
}

void TestWebSocketResponse(CuTest *tc)
{
	char *challenge = "x3JJHMbDL1EzLkh9GBhXDw==";
	char response_buffer[64];
	str_t response_str;
	
	str_init( &response_str, response_buffer, sizeof(response_buffer) );
	
	rq_get_access(challenge, &response_str);
	CuAssertStrEquals_Msg( tc, "challenge/response mismatch",
		"HSmrc0sMlYUkAGmm5OPpG2HaGWk=", str_get_string(&response_str) );
}

void TestHeaderErrors(CuTest *tc)
{
	request_t *req = rq_create();
	char response_buffer[1024];
	str_t response_str;
	int res;
	jsconf_t *conf;

	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");

	str_init( &response_str, response_buffer, sizeof(response_buffer) );

	/* Correct request */

	rq_add_line(req, "GET /mychat HTTP/1.1");
	rq_add_line(req, "Host: server.example.com");
	rq_add_line(req, "Upgrade: websocket");
	rq_add_line(req, "Connection: Upgrade");
	rq_add_line(req, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(req, "Sec-WebSocket-Protocol: xmpp");
	rq_add_line(req, "Sec-WebSocket-Version: 13");
	rq_add_line(req, "Origin: http://firstdomain.com");
	CuAssertTrue(tc, !rq_done(req));
	rq_add_line(req, "");
	
	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue(tc, !rq_is_error(req));
	CuAssertTrue( tc, rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(
		tc,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: HSmrc0sMlYUkAGmm5OPpG2HaGWk=\r\n"
		"Sec-WebSocket-Protocol: xmpp\r\n"
		"\r\n",
		str_get_string(&response_str) );

	/* Wrong HTTP method */
	rq_clear(req);
	rq_add_line(req, "POST /mychat HTTP/1.1");
	rq_add_line(req, "");

	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue( tc, !rq_is_error(req) );
	CuAssertTrue( tc, !rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(                         
		tc,
		"HTTP/1.1 405 Method Not Allowed\r\n"
		"\r\n",
		str_get_string(&response_str) );

	/* TODO: check for wrong protocol, e.g. HTTP/1.0 */

	/* Incorrect origin */
	rq_clear(req);
	rq_add_line(req, "GET /mychat HTTP/1.1");
	rq_add_line(req, "Host: server.example.com");
	rq_add_line(req, "Upgrade: websocket");
	rq_add_line(req, "Connection: Upgrade");
	rq_add_line(req, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(req, "Sec-WebSocket-Protocol: xmpp, smtp");
	rq_add_line(req, "Sec-WebSocket-Version: 13");
	rq_add_line(req, "Origin: http://example.com");
	rq_add_line(req, "");

	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue( tc, !rq_is_error(req) );
	CuAssertTrue( tc, !rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(                         
		tc,
		"HTTP/1.1 403 Forbidden\r\n"
		"\r\n",
		str_get_string(&response_str) );

	/* Client smtp instead of xmpp */
	rq_clear(req);
	rq_add_line(req, "GET /mychat HTTP/1.1");
	rq_add_line(req, "Host: server.example.com");
	rq_add_line(req, "Upgrade: websocket");
	rq_add_line(req, "Connection: Upgrade");
	rq_add_line(req, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(req, "Sec-WebSocket-Protocol: smtp");
	rq_add_line(req, "Sec-WebSocket-Version: 13");
	rq_add_line(req, "Origin: http://firstdomain.com");
	rq_add_line(req, "");

	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue( tc, !rq_is_error(req) );
	CuAssertTrue( tc, !rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(
		tc,
		"HTTP/1.1 400 Bad Request\r\n"
		"\r\n",
		str_get_string(&response_str) );

	/* Client does not support xmpp */
	rq_clear(req);
	rq_add_line(req, "GET /mychat HTTP/1.1");
	rq_add_line(req, "Host: server.example.com");
	rq_add_line(req, "Upgrade: websocket");
	rq_add_line(req, "Connection: Upgrade");
	rq_add_line(req, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(req, "Sec-WebSocket-Version: 13");
	rq_add_line(req, "Origin: http://firstdomain.com");
	rq_add_line(req, "");

	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue( tc, !rq_is_error(req) );
	CuAssertTrue( tc, !rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(
		tc,
		"HTTP/1.1 400 Bad Request\r\n"
		"\r\n",
		str_get_string(&response_str) );

	/* Missing key */
	rq_clear(req);
	rq_add_line(req, "GET /mychat HTTP/1.1");
	rq_add_line(req, "Host: server.example.com");
	rq_add_line(req, "Upgrade: websocket");
	rq_add_line(req, "Connection: Upgrade");
	rq_add_line(req, "Sec-WebSocket-Protocol: xmpp");
	rq_add_line(req, "Sec-WebSocket-Version: 13");
	rq_add_line(req, "Origin: http://firstdomain.com");
	rq_add_line(req, "");

	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue( tc, !rq_is_error(req) );
	CuAssertTrue( tc, !rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(
		tc,
		"HTTP/1.1 400 Bad Request\r\n"
		"\r\n",
		str_get_string(&response_str) );

	/* Wrong host */
	rq_clear(req);
	rq_add_line(req, "GET /mychat HTTP/1.1");
	rq_add_line(req, "Host: example2.com");
	rq_add_line(req, "Upgrade: websocket");
	rq_add_line(req, "Connection: Upgrade");
	rq_add_line(req, "sec-websocket-key: x3JJHMbDL1EzLkh9GBhXDw==");
	rq_add_line(req, "Sec-WebSocket-Protocol: xmpp");
	rq_add_line(req, "Sec-WebSocket-Version: 13");
	rq_add_line(req, "Origin: http://firstdomain.com");
	rq_add_line(req, "");

	CuAssertTrue(tc, rq_done(req));
	CuAssertTrue( tc, !rq_is_error(req) );
	CuAssertTrue( tc, !rq_analyze(req, conf, &response_str) );
	CuAssertStrEquals(
		tc,
		"HTTP/1.1 400 Bad Request\r\n"
		"\r\n",
		str_get_string(&response_str) );

	rq_delete(req);
	config_delete(conf);
}

CuSuite* ParserGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestCreateParser);
	// SUITE_ADD_TEST(suite, TestParseRequestLine);
	SUITE_ADD_TEST(suite, TestTrim);
	SUITE_ADD_TEST(suite, TestParseSimpleRequest);
	SUITE_ADD_TEST(suite, TestParseRequestWithContinuation);
	SUITE_ADD_TEST(suite, TestParseRequiredHeaders);
	SUITE_ADD_TEST(suite, TestWebSocketResponse);
	SUITE_ADD_TEST(suite, TestMultipleProtocols);
	SUITE_ADD_TEST(suite, TestHeaderErrors);
	return suite;
}

