#include "parseconfig.h"
#include <errno.h>
#include <sys/types.h>
#include <regex.h>
#include <fnmatch.h>
#include "log.h"

jsconf_t *
config_create()
{
	jsconf_t *conf = (jsconf_t*) malloc(sizeof(jsconf_t));
	
	if (conf == NULL)
		return NULL;
	memset(conf, 0, sizeof(jsconf_t));
	conf->log_level = LOG_ERR; /* By default, only log errors. */
	return conf;
}

void
config_delete(jsconf_t *conf)
{
	origin_t *current, *next;
	
	current = conf->origin_list;
	while (current != NULL)
	{
		next = current->next;
		free(current->url);
		free(current);
		current = next;
	}
	free(conf->cidr);
	free(conf->port);
	free(conf->host);
	free(conf->resource);
	free(conf);
}

int
config_parse(jsconf_t *conf, const char *file)
{
	FILE *fh = NULL;
	yaml_parser_t parser;
	int parser_initialized = 0;
	yaml_token_t  token;   /* new variable */
	yaml_token_type_t prev_token;
	int level;
	char key[128];
	int res = 0;
	origin_t *new_origin;

	fh = fopen(file, "r");
	if (fh == NULL)
	{
		fprintf( stderr, "Error opening file %s: %s\n", file, strerror(errno) );
		goto Exit;
	}

	/* Initialize parser */
	if (!yaml_parser_initialize(&parser))
		goto Exit;
	parser_initialized = 1;
	
	/* Set input file */
	yaml_parser_set_input_file(&parser, fh);

	prev_token = YAML_NO_TOKEN;
	level = 0;
	do {
		yaml_parser_scan(&parser, &token);
		switch(token.type)
		{
			/* Stream start/end */
			case YAML_STREAM_START_TOKEN:
				break;
			case YAML_STREAM_END_TOKEN:
				break;
			/* Token types (read before actual token) */
			case YAML_KEY_TOKEN:
				break;
			case YAML_VALUE_TOKEN:
				break;
			/* Block delimeters */
			case YAML_BLOCK_SEQUENCE_START_TOKEN:
				level++;
				break;
			case YAML_BLOCK_ENTRY_TOKEN:
				break;
			case YAML_BLOCK_END_TOKEN:
				level--;
				break;
			/* Data */
			case YAML_BLOCK_MAPPING_START_TOKEN:
				level++;
				break;
			case YAML_SCALAR_TOKEN:
				if (prev_token == YAML_KEY_TOKEN)
					strncpy(key, (char*) token.data.scalar.value, sizeof(key));
				if ( (prev_token == YAML_BLOCK_ENTRY_TOKEN) &&
					 (level == 2) &&
					 (strcmp(key, "origin") == 0) )
				{
					new_origin = (origin_t*) malloc(sizeof(origin_t));
					if (new_origin == NULL)
						goto Exit;
					new_origin->url = strdup((char*) token.data.scalar.value);
					sz_tolower(new_origin->url);
					new_origin->next = conf->origin_list;
					conf->origin_list = new_origin;
				}
				if ( (prev_token == YAML_VALUE_TOKEN) &&
					 (level == 1) )
				{
					if (strcmp(key, "port") == 0)
					{
						char *new_port = strdup((char*) token.data.scalar.value);
						if (new_port == NULL)
						{
							LOG(LOG_WARNING,
								"parseconfig.c:config_parse: out of memory");
							break;
						}
						conf->port = new_port;
					}
					else if (strcmp(key, "listen") == 0)
					{
						char *new_cidr = strdup((char*) token.data.scalar.value);
						if (new_cidr == NULL)
						{
							LOG(LOG_WARNING,
								"parseconfig.c:config_parse: out of memory");
							break;
						}
						conf->cidr = new_cidr;
					}
					else if (strcmp(key, "host") == 0)
					{
						char *new_host = strdup((char*) token.data.scalar.value);
						if (new_host == NULL)
						{
							LOG(LOG_WARNING,
								"parseconfig.c:config_parse: out of memory");
							break;
						}
						conf->host = new_host;
					}
					else if (strcmp(key, "resource") == 0)
					{
						char *new_resource = strdup((char*) token.data.scalar.value);
						if (new_resource == NULL)
						{
							LOG(LOG_WARNING,
								"parseconfig.c:config_parse: out of memory");
							break;
						}
						conf->resource = new_resource;
					}
					else if (strcmp(key, "log_level") == 0)
					{
						char *value = (char*) token.data.scalar.value;
						if (strcmp(value, "LOG_EMERG") == 0)
							conf->log_level = LOG_EMERG;
						else if (strcmp(value, "LOG_ALERT") == 0)
							conf->log_level = LOG_ALERT;
						else if (strcmp(value, "LOG_CRIT") == 0)
							conf->log_level = LOG_CRIT;
						else if (strcmp(value, "LOG_ERR") == 0)
							conf->log_level = LOG_ERR;
						else if (strcmp(value, "LOG_WARNING") == 0)
							conf->log_level = LOG_WARNING;
						else if (strcmp(value, "LOG_NOTICE") == 0)
							conf->log_level = LOG_NOTICE;
						else if (strcmp(value, "LOG_INFO") == 0)
							conf->log_level = LOG_INFO;
						else if (strcmp(value, "LOG_DEBUG") == 0)
							conf->log_level = LOG_DEBUG;
					}
					else if (strcmp(key, "max_message_size") == 0)
					{
						conf->max_message_size = atoi((char*) token.data.scalar.value);
					}
					else if (strcmp(key, "max_frame_size") == 0)
					{
						conf->max_frame_size = atoi((char*) token.data.scalar.value);
					}
				}
				break;
			/* Others */
			default:
				fprintf(stderr, "Got unexpected token of type %d\n",
					token.type);
				goto Exit;
		}
		prev_token = token.type;
		if(token.type != YAML_STREAM_END_TOKEN)
		yaml_token_delete(&token);
	} while (token.type != YAML_STREAM_END_TOKEN);
	
	res = 1;

Exit:
	
	if (fh != NULL)
		fclose(fh);
	if (parser_initialized)
		yaml_parser_delete(&parser);
	return res;
}

int
config_check_origin(jsconf_t *conf, const char *origin)
{
	origin_t *current;
	int res;
	
	current = conf->origin_list;
	if (current == NULL) /* If the origin list in the config is empty,
	                        any origin is accepted */
		return 1;

	for (; current != NULL; current = current->next)
	{
		res = fnmatch(current->url, origin, 0);
		if (res == 0)
			return 1;
	}
	return 0;
}

