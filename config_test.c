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
	CuAssertStrEquals(tc, "server.example.com", conf->host);
	CuAssertStrEquals(tc, "/mychat", conf->resource);
	CuAssertIntEquals(tc, 128, conf->max_message_size);
	CuAssertIntEquals(tc, 64, conf->max_frame_size);
	
	/* Check the origin list. */
	current = conf->origin_list;
	CuAssertPtrNotNull(tc, current);
	CuAssertStrEquals(tc, "http://seconddomain.com", current->url);
	
	/* Check the second element in the list. */
	current = current->next;
	CuAssertPtrNotNull(tc, current);
	CuAssertStrEquals(tc, "http://firstdomain.com", current->url);
	
	/* Check that we have reached the end of the list. */
	current = current->next;
	CuAssertTrue(tc, (current == NULL));

	/* Check the log level. */
	CuAssertIntEquals(tc, LOG_INFO, conf->log_level);

	config_delete(conf);
}

void TestCheckOrigin(CuTest *tc)
{
	int res;
	jsconf_t *conf;
	
	conf = config_create();
	CuAssertPtrNotNull(tc, conf);
	
	res = config_parse(conf, "./test/jabsocket-regex.conf");
	CuAssertTrue(tc, res);
	
	CuAssertTrue( tc, config_check_origin(conf, "firstdomain.com") );
	CuAssertTrue( tc, config_check_origin(conf, "seconddomain.com") );
	CuAssertTrue( tc, config_check_origin(conf, "subdomain.seconddomain.com") );
	CuAssertTrue( tc, !config_check_origin(conf, "www.firstdomain.com") );
	CuAssertTrue( tc, !config_check_origin(conf, "other.com") );

	config_delete(conf);

	/* Test with a configuration file with no origin; then any origin domain
	   should be accepted. */

	conf = config_create();
	CuAssertPtrNotNull(tc, conf);
	
	res = config_parse(conf, "./test/jabsocket-noorigin.conf");
	CuAssertTrue(tc, res);

	CuAssertTrue( tc, config_check_origin(conf, "firstdomain.com") );
	CuAssertTrue( tc, config_check_origin(conf, "seconddomain.com") );
	CuAssertTrue( tc, config_check_origin(conf, "subdomain.seconddomain.com") );
	CuAssertTrue( tc, config_check_origin(conf, "subdomain.vogonsoft.com") );

	config_delete(conf);
}

CuSuite* ConfigGetSuite()
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, TestConfig);
	SUITE_ADD_TEST(suite, TestCheckOrigin);
	return suite;
}

