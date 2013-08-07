#include "framer.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

static void XMLCALL framer_start(void *data, const char *el, const char **attr);
static void XMLCALL framer_end(void *data, const char *el);

static void framer_add_frame(framer_t *framer, int index, int size);
static void framer_remove_frame(framer_t *framer);
static int framer_initialize(framer_t *framer);

framer_t *
framer_create()
{
	framer_t *framer = (framer_t*) malloc(sizeof(*framer));
	
	if (framer == NULL)
		goto Error;
	memset(framer, 0, sizeof(*framer));
	framer->parser = XML_ParserCreateNS(NULL, /* ':' */ '\xFF');
	if (framer->parser == NULL)
		goto Error;
	XML_SetElementHandler(
		framer->parser,
		framer_start,
		framer_end);
	XML_SetUserData(
		framer->parser,
		framer);
	framer->level = 0;
	framer->error = 0;
	framer->index = 0;
	framer->buffer_index = 0;
	framer->head = framer->tail = NULL;
	
	/* TODO: create buffers with limited maximum length */
	framer->buffer = buffer_create(0);
	if (framer->buffer == NULL)
		goto Error;

	return framer;

Error:
	if (framer != NULL)
	{
		if (framer->buffer != NULL)
			buffer_delete(framer->buffer);
		if (framer->parser != NULL)
			XML_ParserFree(framer->parser);
	}
	return NULL;
}

static void
framer_cleanup(framer_t *framer)
{
	frame_t *current, *next;

	if (framer->parser != NULL)
	{
		XML_ParserFree(framer->parser);
		framer->parser = NULL;
	}
	if (framer->buffer != NULL)
	{
		buffer_delete(framer->buffer);
		framer->buffer = NULL;
	}
	current = framer->head;
	while (current != NULL)
	{
		next = current->next;
		free(current);
		current = next;
	}
	
}


void
framer_delete(framer_t *framer)
{
	framer_cleanup(framer);
	free(framer);
}

static int
framer_initialize(framer_t *framer)
{
	framer_cleanup(framer);

	framer->parser = XML_ParserCreateNS(NULL, /* ':' */ '\xFF');
	if (framer->parser == NULL)
		goto Error;
	XML_SetElementHandler(
		framer->parser,
		framer_start,
		framer_end);
	XML_SetUserData(
		framer->parser,
		framer);
	framer->level = 0;
	framer->error = 0;
	framer->index = 0;
	framer->buffer_index = 0;
	framer->head = framer->tail = NULL;
	
	framer->buffer = buffer_create(0);
	if (framer->buffer == NULL)
		goto Error;

	return 1;

Error:
	if (framer->parser != NULL)
		XML_ParserFree(framer->parser);
	return 0;
}

int
framer_reset(framer_t *framer)
{
	return framer_initialize(framer);
}

int
framer_add(framer_t *framer, unsigned char *message, size_t length)
{
	enum XML_Status status;

	status = XML_Parse(framer->parser, (char *)message, length, 0);
	if (status == XML_STATUS_ERROR)
	{
		framer->error = 1;
		// printf("Error: %s\n", XML_ErrorString(XML_GetErrorCode(framer->parser) ) );
		return 0;
	}
	buffer_append(framer->buffer, message, length);
	return 1;
}

int
framer_has_frame(framer_t *framer)
{
	return (framer->head != NULL);
}

static void XMLCALL
framer_start(void *data, const char *el, const char **attr)
{
	framer_t *framer = (framer_t*) data;
	char *sep;
	char *name;
	
	// printf("start: element %s level %d\n", el, framer->level);
	sep = strchr(el, '\xFF');
	if (sep != NULL)
	{
		name = sep + 1;
		// printf("   bare element name: %s\n", name);
		if (strcmp(name, "stream") == 0)
		{
			// printf("   Opening stream element\n");
			framer->level = 0; /* Reset level */
			framer_add_frame(
				framer,
				XML_GetCurrentByteIndex(framer->parser),
				XML_GetCurrentByteCount(framer->parser) );
		}
		else if (framer->level == 1)
		{
			// printf("   Stanza opening\n");
			framer->index = XML_GetCurrentByteIndex(framer->parser);
		}
	}
	
	framer->level++;
}

static void
framer_add_frame(framer_t *framer, int index, int size)
{
	frame_t *new_frame = NULL;

	new_frame = (frame_t*) malloc(sizeof(frame_t));
	if (new_frame == NULL)
		goto Exit;
	new_frame->next = NULL;
	if (framer->tail != NULL)
		framer->tail->next = new_frame;
	else
		framer->head = new_frame;
	framer->tail = new_frame;
	new_frame->index = index;
	new_frame->size = size;
	// printf("Adding frame of size %d\n", size);

Exit:
	/* TODO: get into error state if malloc failed. */
	return;
}

static void
framer_remove_frame(framer_t *framer)
{
	frame_t *tmp = framer->head;
	
	if (tmp != NULL)
	{
		framer->head = tmp->next;
		if (framer->head == NULL)
			framer->tail = NULL;
		free(tmp);
	}
}

static void XMLCALL
framer_end(void *data, const char *el)
{
	framer_t *framer = (framer_t*) data;
	int end_index;
	char *sep;
	char *name = NULL;

	framer->level--;
	// printf("end: element %s level %d\n", el, framer->level);

	/* Get the bare element name (without namespace) */
	sep = strchr(el, '\xFF');
	if (sep != NULL)
		name = sep + 1;

	if (framer->level == 1)
	{
		// printf("   Stanza closing\n");
		end_index = XML_GetCurrentByteIndex(framer->parser) +
			XML_GetCurrentByteCount(framer->parser);
		// printf("   Stanza from %d to %d\n", framer->index, end_index);

		/* Add new frame at the end of the list. */
		framer_add_frame(framer, framer->index, end_index - framer->index);
	}
	else if ( (name != NULL) && (strcmp(name, "stream")  == 0) )
	{
		// printf("   Stream closing\n");
		framer_add_frame(
			framer,
			XML_GetCurrentByteIndex(framer->parser),
			XML_GetCurrentByteCount(framer->parser));
	}
}

int
framer_get_frame(framer_t *framer, char **data, size_t *size)
{
	int tmp_index, delta;
	char *tmp_data;
	size_t tmp_size;

	tmp_index = framer->head->index;
	if (tmp_index > framer->buffer_index)
	{
		/* Remove data before stanza (e.g. \r\n between stanzas) */
		delta = tmp_index - framer->buffer_index;
		buffer_remove_data(framer->buffer, delta);
		framer->buffer_index += delta;
	}
	tmp_size = framer->head->size;
	tmp_data = (char*) malloc(tmp_size);
	if (tmp_data == NULL)
		goto Error;
	buffer_get_data(framer->buffer, (unsigned char*) tmp_data, tmp_size);
	buffer_remove_data(framer->buffer, tmp_size);
	framer_remove_frame(framer);
	framer->buffer_index += tmp_size;
	*data = tmp_data;
	*size = tmp_size;

	return 1;

Error:
	return 0;
}

void
framer_get_frame2(framer_t *framer, data_t *data, size_t *size)
{
	int tmp_index, delta;
	size_t tmp_size;

	tmp_index = framer->head->index;
	if (tmp_index > framer->buffer_index)
	{
		/* Remove data before stanza (e.g. \r\n between stanzas) */
		delta = tmp_index - framer->buffer_index;
		buffer_remove_data(framer->buffer, delta);
		framer->buffer_index += delta;
	}
	
	tmp_size = framer->head->size;
	buffer_get_data2(framer->buffer, data, tmp_size);
	
	buffer_remove_data(framer->buffer, tmp_size);
	framer_remove_frame(framer);
	framer->buffer_index += tmp_size;
	*size = tmp_size; /* NOTE: tmp_size may not be equal to data->length:
	                     if the buffer data->buffer has a smaller capacity,
	                     then data->length will be the full capacity of the
	                     buffer (data->capacity), and tmp_size will be the
	                     size of the frame that would have been written to
	                     data->buffer if the buffer was large enough. */
}

