#include "util.h"
#include <string.h>
#include <ctype.h>

/* General functionality */

void
sz_tolower(char *str)
{
	char *pch;
	
	for (pch = str; *pch != '\0'; pch++)
		*pch = tolower(*pch);
}

/* String buffer */

void
str_init(str_t *str, char *buffer, size_t size)
{
	str->buffer = buffer;
	str->length = 0;
	str->capacity = size;
	str->buffer[0] = '\0';
	// memset(buffer, 0, size);
}

void str_clear(str_t *str)
{
	str->length = 0;
	str->buffer[0] = '\0';
}

size_t
str_get_length(str_t *str)
{
	return str->length;
}

size_t
str_get_capacity(str_t *str)
{
	return str->capacity;
}

char *
str_get_string(str_t *str)
{
	return str->buffer;
}

void
str_set_string(str_t *str, const char *fmt, ...)
{
	va_list ap;
	
	va_start(ap, fmt);
	vsnprintf(str->buffer, str->capacity, fmt, ap);
	va_end(ap);
	str->length = strlen(str->buffer);
}

void
str_set_vstring(str_t *str, const char *fmt, va_list ap)
{
	vsnprintf(str->buffer, str->capacity, fmt, ap);
	str->length = strlen(str->buffer);
}

void
str_setn_string(str_t *str, const char *str_in, size_t maxsize)
{
	size_t i;

	if (maxsize >= str->capacity)
		maxsize = str->capacity - 1;
	for (i = 0; i < maxsize; i++)
	{
		if (str_in[i] == '\0')
			break;
		str->buffer[i] = str_in[i];
	}
	str->buffer[i] = '\0';
	str->length = i;
}

void
str_copy_string(str_t *dst, str_t *src)
{
	str_set_string( dst, str_get_string(src) );
}

void
str_trim_beginning(str_t *str, const char *str_in)
{
	const char *pch = str_in;
	
	while (1)
	{
		if (!isspace(*pch))
		{
			str_set_string(str, pch);
			return;
		}
		pch++;
	}
}

void
str_trim_whitespace(str_t *str, const char *str_in)
{
	const char *first = NULL;
	const char *last = NULL;
	const char *ch;
	size_t size;
	
	for (ch = str_in; *ch != '\0'; ch++)\
	{
		if ( !isspace(*ch) )
		{
			if (first == NULL)
				first = ch;
			last = ch;
		}
	}
	if (last != NULL)
	{
		size = last - first + 1;
		str_setn_string(str, first, size);
	}
	else /* str_in contains no non-whitespace characters */
	{
		str_set_string(str, "");
	}
}

void
str_append_char(str_t *str, char ch)
{
	if (str->length + 1 == str->capacity)
		return;
	str->buffer[str->length] = ch;
	str->buffer[str->length + 1] = '\0';
	str->length++;
}

void
str_tolower(str_t *str)
{
	char *pch;
	size_t i;
	
	for (pch = str->buffer, i = 0; i < str->length; i++, pch++)
	{
		*pch = tolower(*pch);
	}
}

int
str_is_equal_nocase(str_t *str, const char *other)
{
	return (strcasecmp(str->buffer, other) == 0);
}

/* Static data buffer */

void
data_init(data_t *data, byte *buffer, size_t size)
{
	data->buffer = buffer;
	data->length = 0;
	data->capacity = size;
	// memset(buffer, 0, size);
}

size_t
data_get_length(data_t *data)
{
	return data->length;
}

size_t
data_get_capacity(data_t *data)
{
	return data->capacity;
}

byte *
data_get_buffer(data_t *data)
{
	return data->buffer;
}

void
data_set_data(data_t *data, byte *data_in, size_t size)
{
	size_t i;

	if (size >= data->capacity)
		size = data->capacity;
	for (i = 0; i < size; i++)
	{
		data->buffer[i] = data_in[i];
	}
	data->length = i;
}

void
unmask(byte *data, size_t length, byte *mask)
{
	size_t i, index;

	index = 0;
	for (i = 0, index = 0; i < length; data++, i++)
	{
		*data = *data ^ mask[index];
		index++;
		if (index == 4)
			index = 0;
	}
}

/* Dynamic data buffer */

#define INIT_BUFFER_SIZE 128

buffer_t *
buffer_create(size_t max_length)
{
	buffer_t *buffer;
	size_t init_capacity;
	
	init_capacity = INIT_BUFFER_SIZE;
	if ( (max_length > 0) && (max_length < init_capacity) )
	{
		init_capacity = max_length;
	}
	
	buffer = (buffer_t*) malloc(sizeof(*buffer));
	if (buffer != NULL)
	{
		buffer->data = (unsigned char*) malloc(init_capacity);
		if (buffer->data == NULL)
			goto Error;
		buffer->length = 0;
		buffer->capacity = init_capacity;
		buffer->max_length = max_length;
	}
	return buffer;

Error:
	if (buffer != NULL)
		free(buffer->data);
	free(buffer);
	return NULL;
}

void
buffer_delete(buffer_t *buffer)
{
	free(buffer->data);
	free(buffer);
}

int
buffer_set_data(buffer_t *buffer, unsigned char *input, size_t length)
{
	buffer->length = 0;
	return buffer_append(buffer, input, length);
}

void
buffer_clear(buffer_t *buffer)
{
	buffer->length = 0;
}

int
buffer_append(buffer_t *buffer, unsigned char *data, size_t length)
{
	unsigned char *new_buff;
	size_t new_cap;
	
	if (length == 0)
		return 1;
	
	if (buffer->max_length == 0) /* unlimited buffer size */
	{
		if (buffer->length + length > buffer->capacity)
		{
			/* Buffer not large enough, we have to resize */
		
			new_cap = buffer->length + length + 1024;
			new_buff = realloc(buffer->data, new_cap);
			if (new_buff == NULL)
				goto Error;
			buffer->data = new_buff;
			buffer->capacity = new_cap;
		}
		if (buffer->length + length > buffer->capacity)
			length = buffer->capacity - buffer->length;
	}
	else /* limited buffer size */
	{
		if (buffer->length + length > buffer->max_length)
			length = buffer->max_length - buffer->length;
		if (length == 0) /* no more room in the buffer */
			goto Error;
		if (buffer->length + length > buffer->capacity)
		{
			new_cap = buffer->length + length + 1024;
			if (new_cap > buffer->max_length)
				new_cap = buffer->max_length;
			new_buff = realloc(buffer->data, new_cap);
			if (new_buff == NULL)
				goto Error;
			buffer->data = new_buff;
			buffer->capacity = new_cap;
		}
	}
	memcpy(buffer->data + buffer->length, data, length);
	buffer->length += length;
	return 1;
	
Error:
	return 0;
}

size_t
buffer_get_length(buffer_t *buffer)
{
	return buffer->length;
}

void
buffer_get_data(buffer_t *buffer, unsigned char *output, size_t length)
{
	/* We assume that the caller has checked that this buffer has enough
	   data available and allocated the memory to receive the data. */
	memcpy(output, buffer->data, length);
}

void
buffer_get_data2(buffer_t *buffer, data_t *data, size_t length)
{
	data_set_data(data, buffer->data, length);
}

void
buffer_peek_data(buffer_t *buffer, unsigned char **data, size_t *length)
{
	*data = buffer->data;
	*length = buffer->length;
}

void
buffer_remove_data(buffer_t *buffer, size_t length)
{
	memmove(buffer->data, buffer->data + length, buffer->capacity - length);
	buffer->length -= length;
}

int
buffer_move(buffer_t *src_buffer, buffer_t *dst_buffer)
{
	buffer_clear(dst_buffer);
	if ( !buffer_append(dst_buffer, src_buffer->data, src_buffer->length) )
		return 0;
	buffer_clear(src_buffer);
	return 1;
}

