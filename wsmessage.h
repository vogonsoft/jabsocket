#ifndef _WSMESSAGE_H_
#define _WSMESSAGE_H_

#include "parseconfig.h"
#include "util.h"
#include "websockframe.h"

/* WebSocket frame opcodes */
#define OPCODE_CONTINUATION  0x00
#define OPCODE_TEXT          0x01
#define OPCODE_BINARY        0x02
#define OPCODE_CLOSE         0x08
#define OPCODE_PING          0x09
#define OPCODE_PONG          0x0a

typedef struct _wsmsg_t_
{
	int frame; /* We have a control frame in the frame buffer */
	int message; /* We have a message in the message buffer */
	int error; /* We have an error (connection has to be terminated) */
	buffer_t *frame_buffer; /* accumulates bytes of a new frame */

	/* Data and parameters of the received frame */
	buffer_t *frame_data; /* unmasked payload of the frame */
	int fin;
	int opcode;
	int mask;

	/* Data and parameters of the received message */
	buffer_t *message_buffer; /* accumulates bytes of a message */
	int message_opcode; /* opcode determines the type: text or binary */
	int message_started; /* =1 if we have already written some data into
	                        message_buffer */

	size_t max_frame_size;
} wsmsg_t;

wsmsg_t *wsmsg_create(jsconf_t *conf);
void wsmsg_delete(wsmsg_t *wsmsg);

int wsmsg_add(wsmsg_t *wsmsg, byte *data, size_t length);
int wsmsg_fail(wsmsg_t *wsmsg);
int wsmsg_has_message(wsmsg_t *wsmsg);
int wsmsg_get_message(wsmsg_t *wsmsg, buffer_t *buffer);
int wsmsg_has_frame(wsmsg_t *wsmsg);
int wsmsg_get_frame(
	wsmsg_t *wsmsg,
	buffer_t *buffer,
	int *fin,
	int *opcode,
	int *mask);

#endif /* _WSMESSAGE_H_ */

