#ifndef _WEBSOCKFRAME_H_
#define _WEBSOCKFRAME_H_

#include <stdlib.h>
#include "util.h"
#include "parseconfig.h"

/* Data buffer */
typedef struct _buffer_t
{
	unsigned char *data;
	size_t length;
	size_t capacity;
	size_t max_length;
} buffer_t;

buffer_t *buffer_create(size_t max_length);
void buffer_delete(buffer_t *buffer);

int buffer_append(buffer_t *buffer, unsigned char *data, size_t length);
size_t buffer_get_length(buffer_t *buffer);
void buffer_get_data(buffer_t *buffer, unsigned char *output, size_t length);
void buffer_get_data2(buffer_t *buffer, data_t *data, size_t length);
void buffer_peek_data(buffer_t *buffer, unsigned char **data, size_t *length);
void buffer_remove_data(buffer_t *buffer, size_t length);

/* WebSocket frame buffer */
typedef struct _wsfbuffer_t
{
	buffer_t *buffer;
} wsfbuffer_t;

int unmask(unsigned char *input, size_t input_len, unsigned char *output,
	size_t *output_len);

wsfbuffer_t *wsfb_create(jsconf_t *conf);
void wsfb_delete(wsfbuffer_t *buffer);

int wsfb_append(wsfbuffer_t *buffer, unsigned char *data, size_t len);
int wsfb_have_message(wsfbuffer_t *buffer);
long wsfb_get_length(wsfbuffer_t *buffer);
int wsfb_get_message(wsfbuffer_t *buffer, int *opcode, unsigned char **data,
	size_t *output_length);

#endif /* _WEBSOCKFRAME_H_ */

