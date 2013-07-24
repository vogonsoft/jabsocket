#ifndef _CMANAGER_H_
#define _CMANAGER_H_

#include "wsserver.h"
#include "streamparse.h"
#include "framer.h"

typedef struct _cmanager_t_
{
	int state;
	wsconn_t *conn;
	streamparser_t *parser;
	char *server; /* URL of the XMPP server */
	struct bufferevent *bev; /* bufferevent for connection with XMPP server */
	struct evdns_base *dnsbase;
	buffer_t *buffer;
	framer_t *framer;
} cmanager_t;

void *cm_create(wsconn_t *conn);
void cm_delete(void *ctx);
int cm_reset(cmanager_t *cm);
void cm_close(cmanager_t *cm);

void cmanager(wsconn_t *conn, int what, void *ctx);
void cm_connect(cmanager_t *cm);

void cm_readcb(struct bufferevent *bev, void *ptr);
void cm_eventcb(struct bufferevent *bev, short events, void *ptr);

#endif /* _CMANAGER_H_ */

