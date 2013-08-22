#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

typedef uint8_t byte;

/* General functionality */
void sz_tolower(char *str);

/* String buffer */

typedef struct _str_t
{
	char *buffer;
	size_t length;
	size_t capacity;
} str_t;

void str_init(str_t *str, char *buffer, size_t size);
void str_clear(str_t *str);
size_t str_get_length(str_t *str);
size_t str_get_capacity(str_t *str);
char *str_get_string(str_t *str);
void str_set_string(str_t *str, const char *fmt, ...);
void str_set_vstring(str_t *str, const char *fmt, va_list ap);
void str_setn_string(str_t *str, const char *str_in, size_t maxsize);

void str_copy_string(str_t *dst, str_t *src);
void str_trim_beginning(str_t *str, const char *str_in);
void str_trim_whitespace(str_t *str, const char *str_in);
void str_append_char(str_t *str, char ch);
void str_tolower(str_t *str);
int str_is_equal_nocase(str_t *str, const char *other);

/* Static data buffer - wrapper for an array on stack */

typedef struct _data_t
{
	byte *buffer;
	size_t length;
	size_t capacity;
} data_t;

void data_init(data_t *data, byte *buffer, size_t size);

size_t data_get_length(data_t *data);
size_t data_get_capacity(data_t *data);
byte *data_get_buffer(data_t *data);
void data_set_data(data_t *data, byte *data_in, size_t size);

/* WebSocket frame unmask */
void unmask(byte *data, size_t length, byte *mask);

/* Dynamic data buffer */

typedef struct _buffer_t
{
	unsigned char *data;
	size_t length;
	size_t capacity;
	size_t max_length;
} buffer_t;

buffer_t *buffer_create(size_t max_length);
void buffer_delete(buffer_t *buffer);

void buffer_clear(buffer_t *buffer);
int buffer_append(buffer_t *buffer, unsigned char *data, size_t length);
size_t buffer_get_length(buffer_t *buffer);
int buffer_set_data(buffer_t *buffer, unsigned char *input, size_t length);
void buffer_get_data(buffer_t *buffer, unsigned char *output, size_t length);
void buffer_get_data2(buffer_t *buffer, data_t *data, size_t length);
void buffer_peek_data(buffer_t *buffer, unsigned char **data, size_t *length);
void buffer_remove_data(buffer_t *buffer, size_t length);
int buffer_move(buffer_t *src_buffer, buffer_t *dst_buffer);

#endif /* _UTIL_H_ */

