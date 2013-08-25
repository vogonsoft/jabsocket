#include <stdlib.h>
#include "CuTest.h"
#include "wsmessage.h"
#include "util.h"

void
TestUnmask(CuTest *tc)
{
	byte mask[] =
		{
			0xb3, 0x11, 0x09, 0xc3 /* mask: b31109c3 */
		};
	unsigned char data[] =
		{
			0xfb, 0x74, 0x65, 0xaf, 0xdc, 0x31, 0x29, 0xf3
				/* masked "Hello  0" */
		};
	buffer_t *buffer;
	unsigned char check_output[9] = "Hello  0";
	int res;

	buffer = buffer_create(0);
	CuAssertTrue( tc, buffer_set_data( buffer, data, sizeof(data) ) );
	CuAssertIntEquals(tc, 8, buffer->length);
	unmask( buffer->data, buffer->length, mask );

	res = memcmp(buffer->data, check_output, buffer->length);
	CuAssertIntEquals(tc, 0, res);
}

void
TestSingleFrameMessage(CuTest *tc)
{
	wsmsg_t *wsmsg;
	jsconf_t *conf;
	int res;
	byte data0[] =
	{
		0x81, 0x88, 0xd9, 0x94, 0x65, 0x3d, 0xb8, 0xf6, 0x6, 0x59, 0xbc, 0xf2,
		0x2, 0x55 
	};
	byte check_output[] =
	{
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68
	};
	buffer_t *buffer;
	int opcode;
	
	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");
	wsmsg = wsmsg_create(conf);
	CuAssertTrue( tc, (wsmsg != NULL) );
	
	wsmsg_add( wsmsg, data0, sizeof(data0) );
	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, wsmsg_has_message(wsmsg) );
	
	buffer = buffer_create(0);
	CuAssertTrue( tc, wsmsg_get_message(wsmsg, buffer, &opcode) );
	CuAssertIntEquals(tc, 8, buffer->length);
	CuAssertIntEquals(tc, OPCODE_TEXT, opcode);
	CuAssertTrue( tc, (memcmp( buffer->data, check_output, 8) == 0) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );
	
	buffer_delete(buffer);
	wsmsg_delete(wsmsg);
	config_delete(conf);
}

void
TestControlFrame(CuTest *tc)
{
	wsmsg_t *wsmsg;
	jsconf_t *conf;
	int res;
	buffer_t *buffer;
	
	/* abcd */
	byte data0[] = {
		0x1, 0x84, 0x43, 0x99, 0x20, 0x8c, 0x22, 0xfb, 0x43, 0xe8 
	};

	/* efgh */
	byte data1[] = {
		0x80, 0x84, 0x1f, 0xc0, 0xa2, 0xa0, 0x7a, 0xa6, 0xc5, 0xc8 
	};

	/* close frame: code=1000=0x03e8, reason="closing" */
	byte close_frame[] = {
		0x88, 0x89, 0x78, 0x79, 0x7a, 0x74, 0x7b, 0x91, 0x19, 0x18, 0x17, 0xa, 0x13, 0x1a, 0x1f 
	};

	/* check_output: "abcdefgh" */
	byte check_output[] =
	{
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68
	};

	int fin;
	int opcode;
	int mask;

	buffer = buffer_create(0);
	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");
	wsmsg = wsmsg_create(conf);
	CuAssertTrue( tc, (wsmsg != NULL) );
	
	wsmsg_add( wsmsg, data0, sizeof(data0) );
	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );

	/* Add Close frame */
	wsmsg_add( wsmsg, close_frame, sizeof(close_frame) );

	/* Add the second frame of the message */
	wsmsg_add( wsmsg, data1, sizeof(data1) );

	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );
	CuAssertTrue( tc, wsmsg_has_frame(wsmsg) );
	res = wsmsg_get_frame(
		wsmsg,
		buffer,
		&fin,
		&opcode,
		&mask);
	CuAssertTrue(tc, res);
	CuAssertTrue(tc, mask);
	CuAssertIntEquals(tc, OPCODE_CLOSE, opcode);

	/* Try again; wsmsg_has_frame should return 0 now */
	CuAssertTrue( tc, !wsmsg_has_frame(wsmsg) );

	/* Now we have a message */
	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, wsmsg_has_message(wsmsg) );

	CuAssertTrue( tc, wsmsg_get_message(wsmsg, buffer, &opcode) );
	CuAssertIntEquals(tc, 8, buffer->length);
	CuAssertIntEquals(tc, OPCODE_TEXT, opcode);
	CuAssertTrue( tc, (memcmp( buffer->data, check_output, 8) == 0) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );

	buffer_delete(buffer);
	wsmsg_delete(wsmsg);
	config_delete(conf);
}

void
TestTwoMessages(CuTest *tc)
{
	/* Message one: "websock" */
	byte messageone0[] = {
		0x1, 0x84, 0xa2, 0x10, 0x90, 0x72, 0xd5, 0x75, 0xf2, 0x1
	};
	byte messageone1[] = {
		0x80, 0x83, 0xba, 0x7e, 0x10, 0x2d, 0xd5, 0x1d, 0x7b 
	};

	/* Message two: "jabber" */
	byte messagetwo0[] = {
		0x1, 0x84, 0x9c, 0x7a, 0xc0, 0x5d, 0xf6, 0x1b, 0xa2, 0x3f
	};
	byte messagetwo1[] = {
		0x80, 0x82, 0xd3, 0xbb, 0x13, 0xee, 0xb6, 0xc9 
	};

	byte check_output_one[] =
	{
		0x77, 0x65, 0x62, 0x73, 0x6f, 0x63, 0x6b
	};

	byte check_output_two[] =
	{
		0x6a, 0x61, 0x62, 0x62, 0x65, 0x72
	};

	wsmsg_t *wsmsg;
	jsconf_t *conf;
	int res;
	buffer_t *buffer;
	int opcode;

	buffer = buffer_create(0);
	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");
	wsmsg = wsmsg_create(conf);

	/* Push first message */

	wsmsg_add( wsmsg, messageone0, sizeof(messageone0) );
	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );

	wsmsg_add( wsmsg, messageone1, sizeof(messageone1) );
	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, wsmsg_has_message(wsmsg) );

	/* Push second message before we read the first one */

	wsmsg_add( wsmsg, messagetwo0, sizeof(messagetwo0) );
	wsmsg_add( wsmsg, messagetwo1, sizeof(messagetwo1) );

	/* Get first message */

	CuAssertTrue( tc, wsmsg_get_message(wsmsg, buffer, &opcode) );
	CuAssertIntEquals(tc, sizeof(check_output_one), buffer->length);
	CuAssertIntEquals(tc, OPCODE_TEXT, opcode);
	CuAssertTrue( tc, (memcmp( buffer->data, check_output_one, sizeof(check_output_one) ) == 0) );
	CuAssertTrue( tc, wsmsg_has_message(wsmsg) );

	/* Get second message */

	CuAssertTrue( tc, wsmsg_get_message(wsmsg, buffer, &opcode) );
	CuAssertIntEquals(tc, sizeof(check_output_two), buffer->length);
	CuAssertIntEquals(tc, OPCODE_TEXT, opcode);
	CuAssertTrue( tc, (memcmp( buffer->data, check_output_two, sizeof(check_output_two) ) == 0) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );

	buffer_delete(buffer);
	wsmsg_delete(wsmsg);
	config_delete(conf);
}

CuSuite* WSMessageGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestUnmask);
	SUITE_ADD_TEST(suite, TestSingleFrameMessage);
	SUITE_ADD_TEST(suite, TestControlFrame);
	SUITE_ADD_TEST(suite, TestTwoMessages);
	return suite;
}

