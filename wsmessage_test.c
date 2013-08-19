#include <stdlib.h>
#include "CuTest.h"
#include "wsmessage.h"
#include "util.h"
#include "websockframe.h"

void TestUnmask(CuTest *tc)
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

void TestSingleFrameMessage(CuTest *tc)
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
	
	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");
	wsmsg = wsmsg_create(conf);
	CuAssertTrue( tc, (wsmsg != NULL) );
	
	wsmsg_add( wsmsg, data0, sizeof(data0) );
	CuAssertTrue( tc, !wsmsg_fail(wsmsg) );
	CuAssertTrue( tc, wsmsg_has_message(wsmsg) );
	
	buffer = buffer_create(0);
	CuAssertTrue( tc, wsmsg_get_message(wsmsg, buffer) );
	CuAssertIntEquals(tc, 8, buffer->length);
	CuAssertTrue( tc, (memcmp( buffer->data, check_output, 8) == 0) );
	CuAssertTrue( tc, !wsmsg_has_message(wsmsg) );
	
	buffer_delete(buffer);
	wsmsg_delete(wsmsg);
	config_delete(conf);
}

/* TODO: tests for multifragment message and control frames */
void TestControlFrame(CuTest *tc)
{
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

}

CuSuite* WSMessageGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestUnmask);
	SUITE_ADD_TEST(suite, TestSingleFrameMessage);
	SUITE_ADD_TEST(suite, TestControlFrame);
	return suite;
}

