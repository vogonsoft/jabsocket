#include <stdlib.h>
#include "CuTest.h"
#include <string.h>
#include "parseconfig.h"
#include <stdio.h>
#include <syslog.h>

void TestConfig(CuTest *tc)
{
	int res;
	jsconf_t *conf;
	origin_t *current;
	
	conf = config_create();
	CuAssertPtrNotNull(tc, conf);
	
	res = config_parse(conf, "./test/jabsocket.conf");
	CuAssertTrue(tc, res);
	CuAssertStrEquals(tc, "5000", conf->port);
	CuAssertStrEquals(tc, "0.0.0.0", conf->cidr);
	
	/* Check the origin list. */
	current = conf->origin_list;
	CuAssertPtrNotNull(tc, current);
	CuAssertStrEquals(tc, "seconddomain.com", current->url);
	
	/* Check the second element in the list. */
	current = current->next;
	CuAssertPtrNotNull(tc, current);
	CuAssertStrEquals(tc, "firstdomain.com", current->url);
	
	/* Check that we have reached the end of the list. */
	current = current->next;
	CuAssertTrue(tc, (current == NULL));

	/* Check the log level. */
	CuAssertIntEquals(tc, LOG_INFO, conf->log_level);

	config_delete(conf);
}

CuSuite* ConfigGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestConfig);
	return suite;
}

