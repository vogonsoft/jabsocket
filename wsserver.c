#include "wsserver.h"
#include <string.h>
#include <event2/buffer.h>
#include "log.h"

static void ws_accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx);
static void wsconn_read_cb(struct bufferevent *bev, void *ctx);
static void wsconn_write_cb(struct bufferevent *bev, void *ctx);

static void wsconn_event_cb(struct bufferevent *bev, short events, void *ctx);

static wsconn_t *wsconn_create(
	wsserver_t *wsserver,
	struct evconnlistener *listener,
	evutil_socket_t fd,
	struct sockaddr *address,
	int socklen);
static void wsconn_delete(wsconn_t *conn);

/* States */
enum _state_t
{
        ST_START,
        ST_RECEIVING,  /* receiving frames */
        ST_CLOSING     /* closing connection */ 
};

wsserver_t *
ws_create(struct event_base *base, void *sin, size_t size)
{
	wsserver_t *wss = 0;

	wss = (wsserver_t*) malloc(sizeof(*wss));
	memset(wss, 0, sizeof(*wss));
	if (wss != NULL)
	{
		memset(wss, 0, sizeof(*wss));
		wss->base = base;
		wss->listener = evconnlistener_new_bind(base, ws_accept_conn_cb, wss,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
			(struct sockaddr*) sin, size);
		if (wss->listener == NULL)
		{
			printf("Couldn't create listener\n");
			goto Error;
		}
		LOG(LOG_INFO, "wsserver.c:ws_create: listening for connections");
	}

	return wss;

Error:
	if (wss != NULL)
	{
		if (wss->listener != NULL)
			evconnlistener_free(wss->listener);
		free(wss);
	}
	return NULL;
}

void
ws_delete(wsserver_t *ws)
{
	if (ws->listener != NULL)
		evconnlistener_free(ws->listener);
	free(ws);
}

static void
ws_accept_conn_cb(struct evconnlistener *listener,
    evutil_socket_t fd, struct sockaddr *address, int socklen,
    void *ctx)
{
	/* We got a new connection! Set up a bufferevent for it. */
	wsserver_t *ws = (wsserver_t*)ctx;
	wsconn_t *conn;
	
	conn = wsconn_create(ws, listener, fd, address, socklen);
	if (conn == NULL)
	{
		LOG(LOG_ERR, "wsserver.c:ws_accept_conn_cb: connection creation failed");
		goto Exit;
	}
	LOG(LOG_DEBUG, "wsserver.c:ws_accept_conn_cb: created connection");
	if ( getnameinfo(address, socklen, conn->host, sizeof(conn->host),
		conn->serv, sizeof(conn->serv), 0) != 0 )
			LOG(LOG_WARNING, "wsserver.c:ws_accept_conn_cb: getnameinfo failed");
	else
		LOG(LOG_INFO, "wsserver.c:ws_accept_conn_cb: connection from host %s:%s",
			conn->host, conn->serv);

Exit:
	return;
}

static void
wsconn_read_cb(struct bufferevent *bev, void *ctx)
{
	wsconn_t *conn = (wsconn_t*) ctx;
	struct evbuffer *input, *output;
	size_t length;
	char *line;
	size_t line_length;
	unsigned char *input_buffer;
	char accept_buffer[64];
	str_t accept_str;

	str_init( &accept_str, accept_buffer, sizeof(accept_buffer) );

	input = bufferevent_get_input(bev);
	output = bufferevent_get_output(bev);
	length = evbuffer_get_length(input);
	
	switch (conn->state)
	{
		case ST_START:
			while (1)
			{
				line = evbuffer_readln(input, &line_length, EVBUFFER_EOL_CRLF);
				if (line != NULL)
				{
					rq_add_line(conn->req, line);
					free(line);
					if ( rq_is_error(conn->req) )
						goto Error;
					if (rq_done(conn->req))
					{
						static char response[4096];

						if (!rq_protocols_contains(conn->req, "xmpp") )
						{
							LOG(LOG_ERR,
								"wsserver.c:wsconn_read_cb: client does not support xmpp");
							goto Error;
						}
						LOG(LOG_DEBUG,
							"wsserver.c:wsconn_read_cb: got the request");
						rq_get_response(
							rq_get_websocket_key(conn->req),
							&accept_str);
						snprintf( response, sizeof(response),
							"HTTP/1.1 101 Switching Protocols\r\n"
							"Upgrade: websocket\r\n"
							"Connection: Upgrade\r\n"
							"Sec-WebSocket-Protocol: xmpp\r\n"
							"Sec-WebSocket-Accept: %s\r\n"
							"\r\n",
							str_get_string(&accept_str) );
						evbuffer_add(output, response, strlen(response));
						conn->state = ST_RECEIVING;
						/* TODO: read any remaining bytes from input and write
						   to output */
						if (conn->cb != NULL)
							conn->cb(conn, WSCB_CONNECTED, conn->custom_ctx);
						break;
					}
				}
				else /* line == NULL */
				{
					LOG(LOG_INFO, "wsserver.c:ws_accept_conn_cb: (%s:%s) evbuffer_readln error",
						conn->host, conn->serv);
					break;
				}
			} /* while (1) */
			break;
		case ST_RECEIVING:
			input_buffer = (unsigned char*) malloc(length);
			if (input_buffer == NULL)
				goto Error;
			evbuffer_remove(input, input_buffer, length);
			wsfb_append(conn->buffer, input_buffer, length);
			free(input_buffer);
			while (wsfb_have_message(conn->buffer))
			{
				int opcode;
				free(conn->message);
				wsfb_get_message(conn->buffer, &opcode, &conn->message,
					&conn->message_length);
				if (conn->cb != NULL)
				{
					if (opcode == 1)
						conn->cb(conn, WSCB_MESSAGE, conn->custom_ctx);
					else if (opcode == 8) /* Close frame */
					{
						uint16_t *status_network;
						
						if (conn->message_length < 2)
						{
							conn->status = (uint16_t) 1001;
							free(conn->reason);
							conn->reason = NULL;
							conn->reason_length = 0;
						}
						else
						{
							status_network = (uint16_t*)conn->message;
							conn->status = ntohs(*status_network);
							if (conn->message_length > 2)
							{
								conn->reason = (unsigned char*)malloc(
									conn->message_length - 2);
								if (conn->reason != NULL)
								{
									memcpy(conn->reason, (conn->message+2),
										conn->message_length - 2);
									conn->reason_length =
										conn->message_length-2;
								}
								else
									conn->reason_length = 0;
							}
						}
						conn->cb(conn, WSCB_CLOSE, conn->custom_ctx);
						wsconn_close(conn, conn->status, conn->reason,
							conn->reason_length);
					}
				}
				free(conn->message);
				conn->message = NULL;
				conn->message_length = 0;
			}
			break;
	}
	return;

Error:
	LOG(LOG_ERR, "wsserver.c:wsconn_read_cb: closing connection");
	free(conn->message);
	conn->message = NULL;
	conn->message_length = 0;
	conn->cb(conn, WSCB_CLOSE, conn->custom_ctx);
	wsconn_close(conn, conn->status, conn->reason,
		conn->reason_length);
}

static void
wsconn_event_cb(struct bufferevent *bev, short events, void *ctx)
{
	wsconn_t *conn = (wsconn_t*) ctx;

	LOG(LOG_INFO, "wsserver.c:wsconn_event_cb (%s:%s) events=%h", conn->host, conn->serv, events);
	if (conn == NULL)
	return;
	if (events & (BEV_EVENT_ERROR|BEV_EVENT_EOF))
	{
		wsconn_delete(conn);
	}
}

static void
wsconn_write_cb(struct bufferevent *bev, void *ctx)
{
	wsconn_t *conn = (wsconn_t*) ctx;
	struct evbuffer *output;
	size_t length;

	output = bufferevent_get_output(bev);
	length = evbuffer_get_length(output);

	if ((conn->state == ST_CLOSING) && (length == 0))
	{
		wsconn_delete(conn);
	}
}

void ws_set_cb(
				wsserver_t *ws,
				ws_create_cb_t create_cb,
				ws_delete_cb_t delete_cb,
				ws_cb_t cb,
				void *ctx)
{
	ws->cb = cb;
	ws->cb_ctx = ctx;
	ws->create_cb = create_cb;
	ws->delete_cb = delete_cb;
}

ws_cb_t ws_get_cb(wsserver_t *ws)
{
	return ws->cb;
}

static void wsconn_write_frame(wsconn_t *conn, uint8_t opcode,
	void *data, size_t size)
{
	int res;
	buffer_t *buffer;
	unsigned char block0[] = { 0 }; /* F + opcode */
	unsigned char block1[9] = { 0 };   /* MASK + length (lower 7 bits + 0, 2, or
										  8 bytes */
	size_t block1_size;
	unsigned char *message;
	size_t message_size;
	struct bufferevent *bev = conn->bev;
	struct evbuffer *output;
	
	block0[0] = 0x80 + (0x0F & opcode);

	buffer = buffer_create();
	if (buffer == NULL)
		goto Exit;
	output = bufferevent_get_output(bev);

	if (buffer == NULL)
		goto Exit;
	res = buffer_append(buffer, block0, 1);
	if (!res)
		goto Exit;

	if (size <= 125)
	{
		/* Size is in the lower nibble of byte 1 */
		block1[0] = size;
		block1_size = 1;
	}
	else if (size < ((2 << 16) - 1))
	{
		/* Size is in bytes 2 and 3 */
		uint16_t u16 = htons((uint16_t) size);
		block1[0] = 126;
		memcpy(&block1[1], &u16, 2);
		block1_size = 3;
	}
	else
	{
		/* Size is in bytes 1-8 */
		uint32_t u32 = htons((uint32_t) size);
		block1[0] = 127;
		memcpy(&block1[5], &u32, 4);
		block1_size = 9;
	}
	res = buffer_append(buffer, block1, block1_size);
	if (!res)
		goto Exit;

	res = buffer_append(buffer, data, size);
	if (!res)
		goto Exit;

	/* Write to output buffer */
	buffer_peek_data(buffer, &message, &message_size);
	evbuffer_add(output, message, message_size);
	buffer_remove_data(buffer, message_size);

Exit:
	if (buffer != NULL)
		buffer_delete(buffer);
	return;
}

void wsconn_write(wsconn_t *conn, void *data, size_t size)
{
	wsconn_write_frame(conn, 1, data, size); /* opcode=1 - text frame */
}

void wsconn_close(wsconn_t *conn, uint16_t status, unsigned char *reason,
	size_t reason_size)
{
	buffer_t *buffer = NULL;
	uint16_t status_network; /* status in network byte order */
	unsigned char *message;
	size_t message_size;
	int res;

	buffer = buffer_create();
	if (buffer == NULL)
		goto Exit;
	if (status > 0)
	{
		status_network = htons(status);
		res = buffer_append(buffer, (unsigned char*)&status_network, 2);
		if (!res)
			goto Exit;
		if ((reason != NULL) && (reason_size > 0))
		{
			res = buffer_append(buffer, reason, reason_size);
			if (!res)
				goto Exit;
		}
	}
	buffer_peek_data(buffer, &message, &message_size);
	wsconn_write_frame(conn, 8, message, message_size); /* opcode=1 - Close frame */
	buffer_remove_data(buffer, message_size);

Exit:
	if (buffer != NULL)
		buffer_delete(buffer);
	conn->state = ST_CLOSING;
}

static wsconn_t *wsconn_create(
	wsserver_t *wsserver,
	struct evconnlistener *listener,
	evutil_socket_t fd,
	struct sockaddr *address,
	int socklen)
{
	wsconn_t *conn;
	struct event_base *base;
	struct bufferevent *bev;
	void *custom_ctx = NULL;
	
	conn = (wsconn_t*) malloc(sizeof(*conn));
	if (conn == NULL)
		goto Error;
	memset(conn, 0, sizeof(*conn));
	conn->wsserver = wsserver;
	conn->cb = wsserver->cb;
	conn->cb_ctx = wsserver->cb_ctx;

	base = evconnlistener_get_base(listener);
	bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	conn->bev = bev;

	conn->state = ST_START;
	conn->req = rq_create();
	if (conn->req == NULL)
	{
		LOG(LOG_ERR, "wsserver.c:wsconn_create Couldn't create request parser\n");
		goto Error;
	}
	conn->buffer = wsfb_create();
	if (conn->buffer == NULL)
	{
		LOG(LOG_ERR, "wsserver.c:wsconn_create Couldn't create websockframe buffer");
		goto Error;
	}

	conn->message = NULL;
	conn->message_length = 0;

	custom_ctx = (wsserver->create_cb)(conn);
	if (custom_ctx == NULL)
		goto Error;
	conn->custom_ctx = custom_ctx;

	bufferevent_setcb(bev, wsconn_read_cb, wsconn_write_cb, wsconn_event_cb, conn);

	bufferevent_enable(bev, EV_READ|EV_WRITE);

	return conn;

Error:
	if (conn != NULL)
	{
		if (custom_ctx != NULL)
			(wsserver->delete_cb)(custom_ctx);
		if (conn->buffer != NULL)
			wsfb_delete(conn->buffer);
		if (conn->req != NULL)
			rq_delete(conn->req);
		free(conn);
	}
	return NULL;
}

static void wsconn_delete(wsconn_t *conn)
{
	if (conn->bev != NULL)
		bufferevent_free(conn->bev);
	if (conn->req != NULL)
		rq_delete (conn->req);
	if (conn->buffer != NULL)
		wsfb_delete(conn->buffer);
	free(conn->message);
	free(conn->reason);
	if (conn->cb != NULL)
		conn->cb(conn, WSCB_DELETE, conn->custom_ctx);
	free(conn);
	LOG(LOG_INFO, "wsserver.c:wsconn_delete (%s:%s) Deleting connection\n",
		conn->host, conn->serv);
}
