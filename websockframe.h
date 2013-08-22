#ifndef _WEBSOCKFRAME_H_
#define _WEBSOCKFRAME_H_

#include <stdlib.h>
#include "util.h"
#include "parseconfig.h"

/* WebSocket frame buffer */
typedef struct _wsfbuffer_t
{
	buffer_t *buffer;
} wsfbuffer_t;

int unmask_l(unsigned char *input, size_t input_len, unsigned char *output,
	size_t *output_len);

wsfbuffer_t *wsfb_create(jsconf_t *conf);
void wsfb_delete(wsfbuffer_t *buffer);

int wsfb_append(wsfbuffer_t *buffer, unsigned char *data, size_t len);
int wsfb_have_message(wsfbuffer_t *buffer);
long wsfb_get_length(wsfbuffer_t *buffer);
int wsfb_get_message(wsfbuffer_t *buffer, int *opcode, unsigned char **data,
	size_t *output_length);

#endif /* _WEBSOCKFRAME_H_ */

