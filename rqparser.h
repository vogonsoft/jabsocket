#ifndef _RQPARSER_H_
#define _RQPARSER_H_

#include "util.h"
#include "parseconfig.h"

struct _strlist_t
{
	char el_buffer[16]; /* Enough capacity for a sub-protocol name */
	str_t el_str;
	struct _strlist_t *next;
};

typedef struct _request_t
{
	int state;
	char *line;
	unsigned int length;
	int done; /* becomes 1 when we are done parsing headers */
	int error; /* becomes 1 when there is a parsing error */

	/* Buffers for parameters obtained from the HTTP request line
	   (method, resource, protocol)
	 */
	char method_buffer[16];
	str_t method_str;
	char resource_buffer[256];
	str_t resource_str;
	char protocol_buffer[16];
	str_t protocol_str;

	int error_code; /* Used for generating HTTP response */

	char host_buffer[256];
	str_t host_str;

	char upgrade_buffer[64];
	str_t upgrade_str;

	char connection_buffer[64];
	str_t connection_str;

	char ws_key_buffer[64];
	str_t ws_key_str;

	char ws_protocol_buffer[64];
	str_t ws_protocol_str;

	struct _strlist_t *protocols_head; /* the head of the list of protocols */
	int protocol_count;

	char ws_version_buffer[32];
	str_t ws_version_str;

	char origin_buffer[256];
	str_t origin_str;
} request_t;

request_t *rq_create();
void rq_delete(request_t *h);
void rq_clear(request_t *h);

void rq_add_line(request_t *h, const char *line);

int rq_done(request_t *h);

/* rq_is_error checks if there are simple errors in parsing the request.
   E.g. a header line with missing colon symbol is a simple error,
   but a bad WebSocket version number is NOT a simple error: for that
   see rq_analyse
 */
int rq_is_error(request_t *h);

/** rq_analyze analyzes the whole header and generates the HTTP response.
    @pre rq_done(h) == 1
    @param h        - request object
    @param conf     - configuration object (needed for checking origin)
    @param response - string object to which the HTTP response is written
    @return
             - 1 - the request was valid
             - 0 - the request was invalid
*/
int rq_analyze(request_t *h, jsconf_t *conf, str_t *response);

char *rq_get_host(request_t *h);
char *rq_get_upgrade(request_t *h);
char *rq_get_connection(request_t *h);
char *rq_get_websocket_key(request_t *h);
char *rq_get_websocket_protocol(request_t *h);
char *rq_get_websocket_version(request_t *h);
char *rq_get_origin(request_t *h);
int rq_get_protocol_count(request_t *h);
char *rq_get_protocol(request_t *h, int index);
int rq_protocols_contains(request_t *h, const char *protocol);

int parse_request_line(request_t *h, const char *line);
char *trim_beginning(char *str);
void rq_get_access(char *challenge, str_t *response);

#endif /* _RQPARSER_H_ */

