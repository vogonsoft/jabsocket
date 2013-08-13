#include <stdlib.h>
#include <stdio.h>
#include "CuTest.h"

CuSuite* ConfigGetSuite();
CuSuite* Base64GetSuite();
CuSuite* ParserGetSuite();
CuSuite* WebSocketFrameGetSuite();
CuSuite* StreamParseGetSuite();
CuSuite* FramerGetSuite();
CuSuite* UtilGetSuite();
CuSuite* WSMessageGetSuite();

void RunAllTests(void) {
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();

	CuSuiteAddSuite(suite, ConfigGetSuite());
	CuSuiteAddSuite(suite, Base64GetSuite());
	CuSuiteAddSuite(suite, ParserGetSuite());
	CuSuiteAddSuite(suite, WebSocketFrameGetSuite());
	CuSuiteAddSuite(suite, StreamParseGetSuite());
	CuSuiteAddSuite(suite, FramerGetSuite());
	CuSuiteAddSuite(suite, UtilGetSuite());
	CuSuiteAddSuite(suite, WSMessageGetSuite());

	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
}

int main(void) {
	RunAllTests();
	return 0;
}

