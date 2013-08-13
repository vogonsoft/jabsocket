#include <stdlib.h>
#include "CuTest.h"
#include "wsmessage.h"

void TestSingleFrameMessage(CuTest *tc)
{
	wsmsg_t *wsmsg;
	jsconf_t *conf;
	int res;
	
	conf = config_create();
	res = config_parse(conf, "./test/jabsocket.conf");
	wsmsg = wsmsg_create(conf);
	CuAssertTrue( tc, (wsmsg != NULL) );
	
	wsmsg_delete(wsmsg);
	config_delete(conf);
}

CuSuite* WSMessageGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestSingleFrameMessage);
	return suite;
}

