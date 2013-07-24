#ifndef _STREAMPARSE_H_
#define _STREAMPARSE_H_

#include <expat.h>

typedef struct _streamparser_t
{
	XML_Parser parser;
	int error;
	char *server;
} streamparser_t;

streamparser_t *streamparser_create();
void streamparser_delete(streamparser_t *parser);
int streamparser_reset(streamparser_t *parser);

int streamparser_add(streamparser_t *parser, const char *data);
int streamparser_is_error(streamparser_t *parser);
int streamparser_has_server(streamparser_t *parser);
const char *streamparser_get_server(streamparser_t *parser);

#endif /* _STREAMPARSE_H_ */

