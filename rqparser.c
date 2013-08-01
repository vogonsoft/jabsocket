#include "rqparser.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <openssl/sha.h>
#include "base64.h"
#include <openssl/sha.h>

#define LINE_BUFFER_SIZE 4096

/* Parser states */
enum State
{
	ST_START,	/* we have not read the first line (GET / HTTP/1.1) yet */
	ST_HEADER,	/* we have read the first line, we are reading headers */
	ST_ERROR	/* input has an error */
};

static void _process(request_t *h, char *key, char *value);
static void _get_protocols(request_t *h);

request_t *
rq_create()
{
	request_t *h;
	
	h = (request_t*) malloc(sizeof(request_t));
	if (h != NULL)
	{
		char *line = (char*) malloc(LINE_BUFFER_SIZE);
		if (line == NULL)
		{
			free(h);
			return NULL;
		}
		h->line = line;
		h->length = 0;
		h->done = 0;
		h->error = 0;
		h->state = ST_START;

		str_init(
			&h->method_str,
			h->method_buffer,
			sizeof(h->method_buffer) );

		str_init(
			&h->resource_str,
			h->resource_buffer,
			sizeof(h->resource_buffer) );

		str_init(
			&h->protocol_str,
			h->protocol_buffer,
			sizeof(h->protocol_buffer) );

		str_init( &h->host_str, h->host_buffer, sizeof(h->host_buffer) );
		str_init( &h->upgrade_str, h->upgrade_buffer, sizeof(h->upgrade_buffer) );
		str_init(
			&h->connection_str,
			h->connection_buffer,
			sizeof(h->connection_buffer) );
		str_init(
			&h->ws_key_str,
			h->ws_key_buffer,
			sizeof(h->ws_key_buffer) );

		str_init(
			&h->ws_protocol_str,
			h->ws_protocol_buffer,
			sizeof(h->ws_protocol_buffer) );

		str_init(
			&h->ws_version_str,
			h->ws_version_buffer,
			sizeof(h->ws_version_buffer) );

		str_init(
			&h->origin_str,
			h->origin_buffer,
			sizeof(h->origin_buffer) );

		h->protocols_head = NULL;
		h->protocol_count = 0;
		h->error_code = 0;
	}
	return h;
}

void
rq_delete(request_t *h)
{
	struct _strlist_t *current, *next;

	/* Delete the list of protocols */
	current = h->protocols_head;
	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}
	
	free(h);
}

void
rq_clear(request_t *h)
{
	h->length = 0;
	h->done = 0;
	h->error = 0;
	h->state = ST_START;

	str_clear(&h->method_str);
	str_clear(&h->resource_str);
	str_clear(&h->protocol_str);

	str_clear(&h->host_str);
	str_clear(&h->upgrade_str);
	str_clear(&h->connection_str);
	str_clear(&h->ws_key_str);
	str_clear(&h->ws_protocol_str);
	str_clear(&h->ws_version_str);
	str_clear(&h->origin_str);
	h->protocols_head = NULL;
	h->protocol_count = 0;
	h->error_code = 0;
}

void
rq_add_line(request_t *h, const char *line)
{
	size_t len = strlen(line);
	int res;
	
	switch (h->state)
	{
		case ST_START:
			res = parse_request_line(h, line);
			if (res == 0)
			{
				h->error = 1;
				return;
			}
			h->state = ST_HEADER;
			h->line[0] = '\0';
			break;
		case ST_HEADER:
			if (len == 0)
			{
				h->done = 1;
			}
			if ( (len == 0) || ( (len > 0) && !isspace(line[0]) ) )
			{
				if (strlen(h->line) > 0)
				{
					/* We have a full line of header, now we can analyze it. */
					char *colon = strchr(h->line, ':');
					size_t keylen;
					char *key;
					char *value;
					if (colon == NULL)
					{
						/* We didn't find a colon. */
						h->error = 1;
						return;
					}
					keylen = colon - h->line;
					key = strndup(h->line, keylen);
					if (key == NULL)
					{
						/* Allocation error */
						h->error = 1;
						return;
					}
					value = strdup(colon + 1);
		
					/* Now we have key and value, process them */
					_process(h, key, value);
				}
			}
			if (len > 0)
			{
				if ( !isspace(line[0]) )
					h->line[0] = '\0';
				else
					line++; /* skip the starting space */
			}
			if (len > 0)
			{
				int plen = strlen(h->line);
				int left = LINE_BUFFER_SIZE - plen - 1;
				char *to = h->line + plen;
				strncpy(to, line, left);
			}
		default:
			break;
	}
}

static void
_process(request_t *h, char *key, char *value)
{
	if (strcasecmp(key, "host") == 0)
	{
		str_trim_beginning(&h->host_str, value);
	}
	else if (strcasecmp(key, "upgrade") == 0)
	{
		str_trim_beginning(&h->upgrade_str, value);
	}
	else if (strcasecmp(key, "connection") == 0)
	{
		str_trim_beginning(&h->connection_str, value);
	}
	else if (strcasecmp(key, "Sec-WebSocket-Key") == 0)
	{
		str_trim_beginning(&h->ws_key_str, value);
	}
	else if (strcasecmp(key, "Sec-WebSocket-Protocol") == 0)
	{
		str_trim_beginning(&h->ws_protocol_str, value);
		_get_protocols(h);
	}
	else if (strcasecmp(key, "Sec-WebSocket-Version") == 0)
	{
		str_trim_beginning(&h->ws_version_str, value);
	}
	else if (strcasecmp(key, "Origin") == 0)
	{
		str_trim_beginning(&h->origin_str, value);
		str_tolower(&h->origin_str);
	}
	free((void*)key);
}

int
rq_done(request_t *h)
{
	return h->done;
}

int
rq_is_error(request_t *h)
{
	return h->error;
}

char *
rq_get_host(request_t *h)
{
	return str_get_string(&h->host_str);
}

char *
rq_get_upgrade(request_t *h)
{
	return str_get_string(&h->upgrade_str);
}

char *
rq_get_connection(request_t *h)
{
	return str_get_string(&h->connection_str);
}

char *
rq_get_websocket_key(request_t *h)
{
	return str_get_string(&h->ws_key_str);
}

char *
rq_get_websocket_protocol(request_t *h)
{
	return str_get_string(&h->ws_protocol_str);
}

char *
rq_get_websocket_version(request_t *h)
{
	return str_get_string(&h->ws_version_str);
}

char *
rq_get_origin(request_t *h)
{
	return str_get_string(&h->origin_str);
}

int
rq_get_protocol_count(request_t *h)
{
	return h->protocol_count;
}

int
parse_request_line(request_t *h, const char *line)
{
	char *line_copy = NULL;
	char *method, *protocol, *resource;
	
	line_copy = strdup(line);
	if (line_copy == NULL)
		return 0;
	
	method = strtok(line_copy, " \t");
	if (method == NULL)
		goto Error;
	str_set_string(&h->method_str, method);

	resource = strtok(NULL, " \t");
	if (resource == NULL)
		goto Error;
	str_set_string(&h->resource_str, resource);

	protocol = strtok(NULL, " \t");
	if (protocol == NULL)
		goto Error;
	str_set_string(&h->protocol_str, resource);
	
	free(line_copy);
	return 1;

Error:
	free(line_copy);
	return 0;
}

char *trim_beginning(char *str)
{
	char *pch = str;
	
	while (1)
	{
		if (!isspace(*pch))
		{
			pch = strdup(pch);
			free(str);
			return pch;
		}
		pch++;
	}
}

#define MAGIC_STRING "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
void rq_get_access(char *challenge, str_t *response)
{
	char buff1[1024];
	unsigned char digest[SHA_DIGEST_LENGTH];
	// char *response;
	size_t output_length;
	
	snprintf(buff1, sizeof(buff1), "%s%s", challenge, MAGIC_STRING);
	SHA1((unsigned char *) buff1, strlen(buff1), (unsigned char *) &digest);

	base64_encode(digest, sizeof(digest), response);
	// response[output_length] = '\0';
	
	// return response;
}

static void
_get_protocols(request_t *h)
{
	char *str, *saveptr, *token;
	char token_nows_buffer[16];
	str_t token_nows_str;
	struct _strlist_t *new_protocol;
	
	str_init( &token_nows_str, token_nows_buffer, sizeof(token_nows_buffer) );
	str = str_get_string(&h->ws_protocol_str);
	while (1)
	{
		token = strtok_r(str, ",", &saveptr);
		str = NULL;
		if (token == NULL)
			break;
		str_trim_whitespace(&token_nows_str, token);
		new_protocol = (struct _strlist_t *) malloc(sizeof(*new_protocol));
		if (new_protocol != NULL)
		{
			str_init(
				&new_protocol->el_str,
				new_protocol->el_buffer,
				sizeof(new_protocol->el_buffer) );
			str_copy_string(&new_protocol->el_str, &token_nows_str);
			new_protocol->next = h->protocols_head;
			h->protocols_head = new_protocol;
			h->protocol_count++;
		}
	}
}

char *
rq_get_protocol(request_t *h, int index)
{
	int i;
	struct _strlist_t *x;

	if ( (index < 0) || (index >= h->protocol_count) )
		return NULL;
	for (i = 0, x = h->protocols_head; i < index; i++, x = x->next)
		;
	return str_get_string(&x->el_str);
}

int
rq_protocols_contains(request_t *h, const char *protocol)
{
	struct _strlist_t *x;
	
	for (x = h->protocols_head; x != NULL; x = x->next)
	{
		if (strcmp(str_get_string(&x->el_str), protocol) == 0)
			return 1;
	}
	return 0;
}

int
rq_analyze(request_t *h, jsconf_t *conf, str_t *response)
{
	str_t accept_str;
	char accept_buffer[64];

	str_init( &accept_str, accept_buffer, sizeof(accept_buffer) );

	if ( !str_is_equal_nocase(&h->method_str, "GET") )
	{
		str_set_string(
			response,
			"HTTP/1.1 405 Method Not Allowed\r\n"
			"\r\n");
		return 0;
	}
	if ( !config_check_origin( conf, rq_get_origin(h) ) )
	{
		str_set_string(
			response,
			"HTTP/1.1 403 Forbidden\r\n"
			"\r\n");
		return 0;
	}
	if ( !rq_protocols_contains(h, "xmpp") )
	{
		str_set_string(
			response,
			"HTTP/1.1 400 Bad Request\r\n"
			"\r\n");
		return 0;
	}

	/* Everything OK, set response to success */
	rq_get_access(
		rq_get_websocket_key(h),
		&accept_str);
	str_set_string( response,
		"HTTP/1.1 101 Switching Protocols\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Accept: %s\r\n"
		"Sec-WebSocket-Protocol: xmpp\r\n"
		"\r\n",
		str_get_string(&accept_str) );

	return 1;
}

