#ifndef _FRAMER_H_
#define _FRAMER_H_

#include <stdlib.h>
#include <expat.h>
#include "websockframe.h"
#include "util.h"

typedef struct _frame_t frame_t;

struct _frame_t
{
	int index; /* Starting index of the frame */
	int size;
	frame_t *next;
};

typedef struct _framer_t
{
	XML_Parser parser;
	int level;
	int error; /* TODO: add error checking */
	int index; /* Starting index of a stanza */
	frame_t *head; /* Head of the list of frames (stanzas) */
	frame_t *tail; /* Tail of the list */
	buffer_t *buffer;
	int buffer_index; /* Starting index of data in the buffer */
} framer_t;

framer_t *framer_create();
void framer_delete(framer_t *framer);
int framer_reset(framer_t *framer);

int framer_add(framer_t *framer, unsigned char *message, size_t length);
int framer_has_frame(framer_t *framer);
int framer_get_frame(framer_t *framer, char **data, size_t *size);
void framer_get_frame2(framer_t *framer, data_t *data, size_t *size);

#endif /* _FRAMER_H_ */

