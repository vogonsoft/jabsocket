#ifndef _WSSERVER_H_
#define _WSSERVER_H_

#include <stdlib.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include "rqparser.h"
#include "parseconfig.h"
#include "util.h"
#include "wsmessage.h"

/* Forward declarations */
typedef struct _wsserver_t wsserver_t;
typedef struct _wsconn_t wsconn_t;

/* Callback types */
/* Creation callback */
typedef void *(*ws_create_cb_t)(wsconn_t *conn);

/* Destruction callback */
typedef void (*ws_delete_cb_t)(void *ctx);

/* Event processing callback */
typedef void (*ws_cb_t)(wsconn_t *conn, int what, void *ctx);


/* Events passed in "what" parameter of the callback */
enum
{
	WSCB_CONNECTED,
	WSCB_MESSAGE,
	WSCB_CLOSE,
	WSCB_DELETE
};

struct _wsserver_t
{
	struct event_base *base;
	struct evconnlistener *listener;
	ws_create_cb_t create_cb;
	ws_delete_cb_t delete_cb;
	ws_cb_t cb;
	void *cb_ctx;
	jsconf_t *conf;
};

struct _wsconn_t
{
	wsserver_t *wsserver;
	int ws_state; /* State of web socket */
	int cm_state; /* State of connection manager */
	int fl_ws_closing; /* Initiated closing of web socket */
	int fl_cm_closed;  /* wsconn_onclosed has been called */
	struct bufferevent *bev;
	request_t *req;
	wsmsg_t *wsmsg; /* Object for processing incoming WebSocket frames */

	buffer_t *message_buffer;

	ws_cb_t cb;
	void *cb_ctx;
	void *custom_ctx;
	
	/* Host address and port as obtained by getnameinfo */
	char host[128];
	char serv[32];
	
	/* Status obtained from Close frame received from the client */
	uint16_t status;
};

wsserver_t *ws_create(struct event_base *base, void *sin, size_t size);
void ws_delete(wsserver_t *ws);

void ws_set_config(wsserver_t *ws, jsconf_t *conf);

void ws_set_cb(
				wsserver_t *ws,
				ws_create_cb_t create_cb,
				ws_delete_cb_t delete_cb,
				ws_cb_t cb,
				void *ctx);

ws_cb_t ws_get_cb(wsserver_t *ws);

void wsconn_write(wsconn_t *conn, void *data, size_t size);
void wsconn_handshake(wsconn_t *conn);
void wsconn_initiate_close(wsconn_t *conn, uint16_t status, unsigned char *reason,
	size_t reason_size);
void wsconn_close(wsconn_t *conn);

/* wsconn_close_send does not close the connection immediately,
   but marks it for close. The connection will be closed after all output data
   has been sent out. We use this for example when we have to send an error
   response to the browser before closing the connection.
 */
void wsconn_close_send(wsconn_t *conn);

/* wsconn_onclose is called from CM when the connection is closed */
void wsconn_onclosed(wsconn_t *conn);

#endif /* _WSSERVER_H_ */

