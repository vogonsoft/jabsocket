#include <stdlib.h>
#include "CuTest.h"
#include <string.h>
#include "websockframe.h"
#include "util.h"

void TestGetFrame(CuTest *tc)
{
	/* Note that input[] is a complete WebSocket frame: flags, opcode,
	   mask flag, length, mask, payload.
	   Length is 8, so there is no extended length.
	 */
	unsigned char input[] =
		{
			0x81, 0x88,	/* 81: FIN=1, opcode=1 (text); 88: mask=1, length=8 */
			0xb3, 0x11, 0x09, 0xc3, /* mask: b31109c3 */
			0xfb, 0x74, 0x65, 0xaf, 0xdc, 0x31, 0x29, 0xf3
				/* masked "Hello  0" */
		};
	wsfbuffer_t *buffer;
	int opcode;
	size_t length;
	int res;
	unsigned char *message;
	unsigned char check_message[] =
		{
			0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 0x20, 0x30
		};
	long res_length;
	jsconf_t *conf;

	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");
	
	buffer = wsfb_create(conf);
	CuAssertTrue(tc, buffer != NULL);

	/* First we add the first 5 bytes, so there is not a full message in
	   the buffer yet. */	
	wsfb_append(buffer, &input[0], 5);
	res = wsfb_have_message(buffer);
	CuAssertTrue(tc, !res);

	wsfb_append(buffer, &input[5], 9);
	res = wsfb_have_message(buffer);
	CuAssertTrue(tc, res);
	
	length = wsfb_get_length(buffer);
	CuAssertIntEquals(tc, 8, length);

	/* Check passing NULL for message when we are only interested in
	   message length. */
	length = 0;
	opcode = 0;
	res = wsfb_get_message(buffer, &opcode, NULL, &length);
	CuAssertTrue(tc, res);
	CuAssertIntEquals(tc, 8, length);
	CuAssertIntEquals(tc, 1, opcode);

	/* Get the message and check that the buffer is empty after that. */
	message = NULL;
	res = wsfb_get_message(buffer, &opcode, &message, &length);
	CuAssertTrue(tc, res);
	CuAssertIntEquals(tc, 0, memcmp(check_message, message, 8) );
	CuAssertIntEquals(tc, 8, length);
	CuAssertIntEquals(tc, 1, opcode);
	free(message);
	res_length = wsfb_get_length(buffer);
	CuAssertIntEquals(tc, -1, res_length);
	
	wsfb_delete(buffer);
}

CuSuite* WebSocketFrameGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestGetFrame);
	return suite;
}

