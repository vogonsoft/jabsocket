#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdlib.h>
#include <stdint.h>

typedef uint8_t byte;

/* String buffer */

typedef struct _str_t
{
	char *buffer;
	size_t length;
	size_t capacity;
} str_t;

void str_init(str_t *str, char *buffer, size_t size);
size_t str_get_length(str_t *str);
size_t str_get_capacity(str_t *str);
char *str_get_string(str_t *str);
void str_set_string(str_t *str, const char *str_in);
void str_setn_string(str_t *str, const char *str_in, size_t maxsize);

void str_copy_string(str_t *dst, str_t *src);
void str_trim_beginning(str_t *str, const char *str_in);
void str_trim_whitespace(str_t *str, const char *str_in);
void str_append_char(str_t *str, char ch);

/* Data buffer */

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

#endif /* _UTIL_H_ */

