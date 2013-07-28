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
str_set_string(str_t *str, const char *str_in)
{
	str_setn_string(str, str_in, str->capacity);
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

/* Data buffer */

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

