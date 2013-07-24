#include "streamparse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static int streamparser_initialize(streamparser_t *parser);

static void XMLCALL
streamparser_start(void *data, const char *el, const char **attr)
{
	streamparser_t *parser = (streamparser_t*) data;
	char *separator;
	char *namespace;
	char *element; /* element without namespace */
	size_t before;
	
	// printf("streamparser_start: el=%s\n", el);
	separator = strchr(el, '\xFF');
	if (separator != NULL)
	{
		/* Since we created the XML parser by calling XML_ParserCreateNS,
		   el will be a string of the form "namespace\xFFelement",
		   for example "http://etherx.jabber.org/streams\xFFstream",
		   so we have to extract the part before \xFF to namespace and
		   the part after \xFF to element name.
		*/
		before = separator - el;
		if (parser->server == NULL)
		{
			namespace = strndup(el, before);
			element = strdup(separator + 1);
			// printf("namespace: %s element: %s\n", namespace, element);
			if ((strcmp(namespace, "http://etherx.jabber.org/streams") == 0) &&
				(strcmp(element, "stream") == 0))
			{
				for (; *attr != NULL; attr += 2)
				{
					// printf("attr: %s value: %s\n", *attr, *(attr+1));
					if (strcmp(*attr, "to") == 0)
					{
						parser->server = strdup(*(attr + 1));
						break;
					}
				}
			}
			free(namespace);
			free(element);
		}
	}
}

static void XMLCALL
streamparser_end(void *data, const char *el)
{
	// streamparser_t *parser = (streamparser_t*) data;
	// printf("streamparser_end: el=%s\n", el);
	(void) data;
	(void) el;
}

void XMLCALL
streamparser_start_namespace(
	void *userData,
	const XML_Char *prefix,
	const XML_Char *uri)
{
	// streamparser_t *parser = (streamparser_t*) userData;
	// printf("streamparser_start_namespace: prefix=%s uri=%s\n",
	//	prefix, uri);
	(void) userData;
	(void) prefix;
	(void) uri;
}

void XMLCALL
streamparser_end_namespace(
	void *userData,
	const XML_Char *prefix)
{
	// streamparser_t *parser = (streamparser_t*) userData;
	// printf("streamparser_end_namespace: prefix=%s\n", prefix);
	(void) userData;
	(void) prefix;
}

static int
streamparser_initialize(streamparser_t *parser)
{
	if (parser->parser != NULL)
		XML_ParserFree(parser->parser);
	parser->parser = XML_ParserCreateNS(NULL, /* ':' */ '\xFF');
	if (parser->parser == NULL)
		goto Error;

	XML_SetElementHandler(
		parser->parser,
		streamparser_start,
		streamparser_end);
	XML_SetNamespaceDeclHandler(
		parser->parser,
		streamparser_start_namespace,
		streamparser_end_namespace);
	XML_SetUserData(
		parser->parser,
		parser);

	parser->error = 0;
	return 1;

Error:
	return 0;
}

streamparser_t *
streamparser_create()
{
	streamparser_t *parser;
	
	parser = (streamparser_t*) malloc(sizeof(*parser));
	if (parser == NULL)
		goto Error;
	memset(parser, 0, sizeof(*parser));
	if (!streamparser_initialize(parser))
		goto Error;

	parser->server = NULL;
	return parser;

Error:
	if (parser != NULL)
	{
		free(parser);
	}
	return NULL;
}

void
streamparser_delete(streamparser_t *parser)
{
	XML_ParserFree(parser->parser);
	free(parser->server);
	free(parser);
}

int
streamparser_reset(streamparser_t *parser)
{
	return streamparser_initialize(parser);
}

int
streamparser_add(streamparser_t *parser, const char *data)
{
	enum XML_Status status;

	status = XML_Parse(parser->parser, data, strlen(data), 0);
	if (status == XML_STATUS_ERROR)
	{
		parser->error = 1;
		return 0;
	}
	return 1;
}

int
streamparser_is_error(streamparser_t *parser)
{
	return parser->error;
}

int
streamparser_has_server(streamparser_t *parser)
{
	return (parser->server != NULL);
}

const char *
streamparser_get_server(streamparser_t *parser)
{
	return parser->server;
}

