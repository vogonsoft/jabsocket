#include <stdlib.h>
#include "CuTest.h"
#include <string.h>
#include "framer.h"
#include <stdio.h>
#include "util.h"

/*
static void
show_bytes(char *data, size_t size)
{
	size_t i;
	
	printf("Frame:\n");
	for (i = 0; i < size; i++)
	{
		printf("%c", data[i]);
	}
	printf("\n");
}
*/

void TestFramer(CuTest *tc)
{
	framer_t *framer;
	int res;
	char *message;
	char *data;
	size_t size;
	
	framer = framer_create();
	CuAssertPtrNotNull(tc, framer);

	message = 
		"<stream:stream xmlns:stream=\"http://etherx.jabber.org/streams\"\r\n"
		"	xmlns=\"jabber:client\"\r\n"
		"	from=\"cloud01\"\r\n"
		"	id=\"e26f1f04\"\r\n"
		"	xml:lang=\"en\"\r\n";

	res = framer_add(framer, (unsigned char *) message, strlen(message));
	CuAssertTrue(tc, res);
	res = framer_has_frame(framer);
	CuAssertTrue(tc, !res);
	
	message =
		"	version=\"1.0\">\r\n"
		"<stream:features>\r\n"
		"	<compression\r\n"
		"		xmlns=\"http://jabber.org/features/compress\">\r\n"
		"		<method>zlib</method>\r\n"
		"	</compression>\r\n"
		"	<bind\r\n"
		"		xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\"/>\r\n"
		"	<session\r\n"
		"		xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>\r\n"
		"</stream:features>\r\n"
		"</stream:stream>"
		;
	res = framer_add(framer, (unsigned char *) message, strlen(message));
	CuAssertTrue(tc, res);
	res = framer_has_frame(framer);
	CuAssertTrue(tc, res);
	
	/* Get stream frame */
	res = framer_get_frame(framer, &data, &size);
	CuAssertTrue(tc, res);
	CuAssertTrue(tc, (memcmp(data, "<stream:stream xmlns:stream=", 28) == 0) );
	// show_bytes(data, size);

	/* Get features stanza */
	res = framer_has_frame(framer);
	CuAssertTrue(tc, res);
	res = framer_get_frame(framer, &data, &size);
	CuAssertTrue(tc, res);
	CuAssertTrue(tc, (memcmp(data, "<stream:features>", 17) == 0) );
	// show_bytes(data, size);

	/* Get </stream:stream> */
	res = framer_has_frame(framer);
	CuAssertTrue(tc, res);
	res = framer_get_frame(framer, &data, &size);
	CuAssertTrue(tc, res);
	CuAssertTrue(tc, (memcmp(data, "</stream:stream>", 17) == 0) );
	// show_bytes(data, size);

	/* We are at the end now */
	res = framer_has_frame(framer);
	CuAssertTrue(tc, !res);
}

/* Testing framer_get_frame2: reading a frame into data_t */
void TestFramer2(CuTest *tc)
{
	framer_t *framer;
	int res;
	char *message;
	byte data_buffer[1024];
	data_t data;
	size_t size;
	
	data_init( &data, data_buffer, sizeof(data_buffer) );
	
	framer = framer_create();
	CuAssertPtrNotNull(tc, framer);

	message = 
		"<stream:stream xmlns:stream=\"http://etherx.jabber.org/streams\"\r\n"
		"	xmlns=\"jabber:client\"\r\n"
		"	from=\"cloud01\"\r\n"
		"	id=\"e26f1f04\"\r\n"
		"	xml:lang=\"en\"\r\n";

	res = framer_add(framer, (unsigned char *) message, strlen(message));
	
	message =
		"	version=\"1.0\">\r\n"
		"<stream:features>\r\n"
		"	<compression\r\n"
		"		xmlns=\"http://jabber.org/features/compress\">\r\n"
		"		<method>zlib</method>\r\n"
		"	</compression>\r\n"
		"	<bind\r\n"
		"		xmlns=\"urn:ietf:params:xml:ns:xmpp-bind\"/>\r\n"
		"	<session\r\n"
		"		xmlns=\"urn:ietf:params:xml:ns:xmpp-session\"/>\r\n"
		"</stream:features>\r\n"
		"</stream:stream>"
		;
	res = framer_add(framer, (unsigned char *) message, strlen(message));
	
	/* Get stream frame */
	framer_get_frame2(framer, &data, &size);
	
	CuAssertTrue(tc, (memcmp(data_get_buffer(&data), "<stream:stream xmlns:stream=", 28) == 0) );
	CuAssertIntEquals(tc, 152, size);
	CuAssertIntEquals( tc, 152, data_get_length(&data) );
}

CuSuite* FramerGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestFramer);
	SUITE_ADD_TEST(suite, TestFramer2);
	return suite;
}

