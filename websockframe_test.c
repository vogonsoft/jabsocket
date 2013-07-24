#include <stdlib.h>
#include "CuTest.h"
#include <string.h>
#include "websockframe.h"
#include "util.h"

void TestUnmask(CuTest *tc)
{
	/* Note that input[] is not a complete WebSocket frame, only mask
	   and payload */
	unsigned char input[] =
		{
			0xb3, 0x11, 0x09, 0xc3, /* mask: b31109c3 */
			0xfb, 0x74, 0x65, 0xaf, 0xdc, 0x31, 0x29, 0xf3
				/* masked "Hello  0" */
		};
	unsigned char output[9]; /* for "Hello  0\0" */
	unsigned char check_output[9] = "Hello  0";
	int res;
	size_t out_len;

	out_len = sizeof(output);
	res = unmask(input, sizeof(input), output, &out_len);
	CuAssertTrue(tc, res);
	CuAssertIntEquals(tc, 8, out_len);
	res = memcmp(output, check_output, out_len);
	CuAssertIntEquals(tc, 0, res);
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
	
	buffer = buffer_create();
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
	
	buffer = buffer_create();
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
	
	buffer = wsfb_create();
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
	SUITE_ADD_TEST(suite, TestUnmask);
	SUITE_ADD_TEST(suite, TestBuffer);
	SUITE_ADD_TEST(suite, TestBuffer2);
	SUITE_ADD_TEST(suite, TestGetFrame);
	return suite;
}

