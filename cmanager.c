#include "cmanager.h"
#include <stdio.h>
#include <string.h>
#include <event2/dns.h>
#include <event2/buffer.h>
#include <errno.h>

typedef enum _cm_state_t
{
	ST_START,   /* Receiving initial <stream> element from browser */
	ST_CONNECT, /* Connecting with XMPP server */
	ST_FORWARD  /* Forwarding from browser to XMPP server and vice versa */
} cm_state_t;


static void cm_onmessage(
	cmanager_t *cm,
	unsigned char *message,
	size_t message_length);

void *
cm_create(wsconn_t *conn)
{
	cmanager_t *cm = (cmanager_t*) malloc(sizeof(*cm));
	memset(cm, 0, sizeof(*cm));
	if (cm != NULL)
	{
		cm->conn = conn;
		cm->state = ST_START;
		cm->parser = streamparser_create();
		if (cm->parser == NULL)
			goto Error;
		cm->server = NULL;
		cm->bev = NULL;
		cm->dnsbase = NULL;
		cm->buffer = buffer_create(0); /* TODO: limited size buffer */
		if (cm->buffer == NULL)
			goto Error;
		cm->framer = framer_create();
		if (cm->framer == NULL)
			goto Error;
		return cm;
	}

Error:
	if (cm != NULL)
	{
		free(cm->framer);
		free(cm->buffer);
		free(cm->parser);
		free(cm);
	}
	return NULL;
}

int
cm_reset(cmanager_t *cm)
{
	return streamparser_reset(cm->parser);
}

void
cm_delete(void *ctx)
{
	cmanager_t *cm = (cmanager_t*) ctx;
	cm_close(ctx);
	free(cm);
}

void
cm_close(cmanager_t *cm)
{
	if (cm->framer != NULL)
	{
		framer_delete(cm->framer);
		cm->framer = NULL;
	}
	if (cm->buffer != NULL)
	{
		buffer_delete(cm->buffer);
		cm->buffer = NULL;
	}
	if (cm->parser != NULL)
	{
		streamparser_delete(cm->parser);
		cm->parser = NULL;
	}
	if (cm->bev != NULL)
	{
		bufferevent_free(cm->bev);
		cm->bev = NULL;
	}
	if (cm->dnsbase != NULL)
	{
		evdns_base_free(cm->dnsbase, 1);
		cm->dnsbase = NULL;
	}
	if (cm->server != NULL)
	{
		free(cm->server);
		cm->server = NULL;
	}
}

static void
show_bytes(unsigned char *buf, size_t len)
{
	size_t i, j;

	j = 0;
	for (i = 0; i < len; i++)
	{
		// printf("%02x(%c)", buf[i], buf[i]);
		printf("%c", buf[i]);
		j++;
		if (j == 16)
		{
			printf("\n");
			j = 0;
		}
		else
		{
			printf(" ");
		}
	}
	if (j == 0)
		printf("\n");
	fflush(stdout);
}

void
cmanager(wsconn_t *conn, int what, void *ctx)
{
	cmanager_t *cm = (cmanager_t*) ctx;

	switch (what)
	{
		case WSCB_CONNECTED:
			break;
		case WSCB_MESSAGE:
			cm_onmessage(cm, conn->message, conn->message_length);
			break;
		case WSCB_CLOSE:
			cm_close(cm);
			break;
		case WSCB_DELETE:
			cm_delete(cm);
	}
}

static void
cm_onmessage(cmanager_t *cm, unsigned char *message, size_t message_length)
{
	const char *server;
	char *message_str = NULL; /* We need this because message may not be
								 zero-terminated. */

	switch (cm->state)
	{
		case ST_START:
			message_str = strndup((char*) message, message_length);
			streamparser_add(cm->parser, (char*) message_str);
			free(message_str);
			if (streamparser_is_error(cm->parser))
			{
				/* TODO */
				return;
			}
			buffer_append(cm->buffer, message, message_length);
			if (streamparser_has_server(cm->parser))
			{
				server = streamparser_get_server(cm->parser);
				// printf("Got server: %s\n", server);
				cm->server = strdup(server);
				cm_connect(cm);
				cm->state = ST_CONNECT;
			}
			break;
		case ST_FORWARD:
			bufferevent_write(cm->bev, message, message_length);
			// show_bytes(message, message_length);
			break;
	}
}

void
cm_connect(cmanager_t *cm)
{
	/* Initiate conect with cm->server via cm->bev */
	struct event_base *base;

	base = cm->conn->wsserver->base;
	cm->bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
	if (cm->bev == NULL)
		goto Error;
	bufferevent_setcb(cm->bev, cm_readcb, NULL, cm_eventcb, cm);
	bufferevent_enable(cm->bev, EV_READ|EV_WRITE);
	
	cm->dnsbase = evdns_base_new(base, 1);
	if (cm->dnsbase == NULL)
		goto Error;

	/* TODO: how do we get a nonstandard XMPP port (not 5222)? */
	if (bufferevent_socket_connect_hostname(cm->bev,
		cm->dnsbase, AF_UNSPEC, cm->server, 5222) < 0)
			goto Error;

Error:
	return;
}

static int
is_prefix(char *prefix, char *str)
{
	size_t n;

	if (strlen(prefix) > strlen(str))
		return 0;
	n = strlen(prefix);
	return (strncmp(str, prefix, n) == 0);
}

void
cm_readcb(struct bufferevent *bev, void *ptr)
{
	cmanager_t *cm = (cmanager_t*) ptr;
	struct evbuffer *input = bufferevent_get_input(bev);
	char buf[1025]; /* Buffer for 1024 characters + null termination */
	int n;
	int res;
	int begin = 1;
	data_t data;
	static byte data_buffer[65536];

	data_init( &data, data_buffer, sizeof(data_buffer) );

	while ((n = evbuffer_remove(input, buf, 1024)) > 0)
	{
		// printf("\nParsing data from XMPP server:\n");
		// show_bytes((unsigned char *) buf, n);

		/* If the stream is restarted (after TLS negotiation or
		   authentication, reset the framer. */
		buf[n] = '\0'; /* Add null byte at the end of string, just in case. */
		if (begin && 
			( is_prefix("<?xml", buf) ||
			  is_prefix("<stream:stream", buf) ) )
		{
			framer_reset(cm->framer);
		}

		res = framer_add(cm->framer, (unsigned char *) buf, n);
		begin = 0;
		if (!res)
			return; /* TODO: set error */
	}
	while (framer_has_frame(cm->framer))
	{
		size_t frame_size;
		framer_get_frame2(cm->framer, &data, &frame_size);
		wsconn_write( cm->conn, data_get_buffer(&data), data_get_length(&data) );
	}
}

void
cm_eventcb(struct bufferevent *bev, short events, void *ptr)
{
	cmanager_t *cm = (cmanager_t*) ptr;
	unsigned char *data;
	size_t length;

	if (events & BEV_EVENT_CONNECTED)
	{
		buffer_peek_data(cm->buffer, &data, &length);
		bufferevent_write(bev, data, length);
		buffer_remove_data(cm->buffer, length);
		cm->state = ST_FORWARD;
		return;
	}
	if (events & BEV_EVENT_ERROR)
	{
		char *reason = "XMPP connection failed";
		printf("   BEV_EVENT_ERROR: %s\n",
			evutil_socket_error_to_string(EVUTIL_SOCKET_ERROR() ) );
		wsconn_onclosed(cm->conn);
		cm_close(cm);
	}
}


