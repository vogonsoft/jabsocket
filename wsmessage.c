#include "wsmessage.h"

wsmsg_t *
wsmsg_create(jsconf_t *conf)
{
	wsmsg_t *wsmsg;
	
	wsmsg = (wsmsg_t *) malloc( sizeof(wsmsg_t) );
	if (wsmsg == NULL)
		goto Error;
	memset( wsmsg, 0, sizeof(wsmsg_t) );
	wsmsg->frame = 0;
	wsmsg->message = 0;
	wsmsg->error = 0;
	wsmsg->frame_buffer = buffer_create(2 * conf->max_frame_size);
	if (wsmsg->frame_buffer == NULL)
		goto Error;
	wsmsg->message_buffer = buffer_create(conf->max_message_size);
	if (wsmsg->message_buffer == NULL)
		goto Error;
	wsmsg->frame_data = buffer_create(conf->max_frame_size);
	if (wsmsg->frame_data == NULL)
		goto Error;
	wsmsg->max_frame_size = conf->max_frame_size;
	
	wsmsg->message_opcode = 0;
	wsmsg->message_started = 0;

	return wsmsg;

Error:
	if (wsmsg != NULL)
	{
		if (wsmsg->frame_data != NULL)
			buffer_delete(wsmsg->frame_data);
		if (wsmsg->message_buffer != NULL)
			buffer_delete(wsmsg->message_buffer);
		if (wsmsg->frame_buffer != NULL)
			buffer_delete(wsmsg->frame_buffer);
		free(wsmsg);
	}
	return NULL;
}

void
wsmsg_delete(wsmsg_t *wsmsg)
{
	if (wsmsg != NULL)
	{
		if (wsmsg->frame_data != NULL)
			buffer_delete(wsmsg->frame_data);
		if (wsmsg->message_buffer != NULL)
			buffer_delete(wsmsg->message_buffer);
		if (wsmsg->frame_buffer != NULL)
			buffer_delete(wsmsg->frame_buffer);
		free(wsmsg);
	}
}

/* _frame_status_t: status of the buffer in which we are accumulating a frame
   (still building, ready, error)
 */
enum _frame_status_t
{
	FS_BUILDING,	/* still building a frame */
	FS_READY,		/* a frame is ready in the buffer */
	FS_ERROR		/* there is an error with the data in the buffer */
};

enum _frame_status_t
get_frame(
	buffer_t *src_buffer,
	int *fin, /* FIN flag */
	int *opcode,
	int *mask, /* MASK flag */
	buffer_t *buffer /* buffer that will get the frame payload */
	)
{
	int opcode_tmp;
	int fin_tmp;
	size_t message_length;
	size_t header_length;
	size_t frame_length;
	int mask_tmp;
	byte *message;
	byte *data = src_buffer->data;
	size_t length = src_buffer->length;
	
	if (length < 2)
		return FS_BUILDING;
	if ( (data[0] & 0x70) != 0 ) /* RSV1-3 must be 0 */
		return FS_ERROR;
	opcode_tmp = data[0] & 0x0f;
	fin_tmp = 0;
	if ( (data[0] & 0x80) != 0 )
		fin_tmp = 1;
	message_length = data[1] & 0x7f;
	mask_tmp = 0;
	if ( (data[1] & 0x80) != 0 )
		mask_tmp = 1;
	if (message_length == 126)
	{
		/* RFC-6455: payload length is encoded in next two bytes (2 and 3) */
		uint16_t netshort;
		if (length < 4)
			goto Exit;
		netshort = *(uint16_t*) (data + 2);
		message_length = ntohs(netshort);
		header_length = 4;
	}
	else if (message_length == 127)
	{
		/* RFC-6455: payload length is encoded in next 8 bytes (2 to 9) */
		/* In the current implementation, we don't deal with message lengths
		   that don't fit in 4 bytes */
		uint32_t netlong;
		if (length < 10)
			goto Exit;
		netlong = *(uint32_t*) (data + 6);
		message_length = ntohl(netlong);
		header_length = 10;
	}
	else
		header_length = 2;
	
	/* Copy message_length bytes from buffer+header_length to buffer */
	if (mask_tmp)
	{
		message = data + header_length + 4;
		frame_length = header_length + 4 + message_length;
	}
	else
	{
		message = data + header_length;
		frame_length = header_length + message_length;
	}
	if ( !buffer_set_data(buffer, message, message_length) )
		return FS_ERROR;
	
	if (frame_length > length) /* We don't yet have the whole frame */
		goto Exit;

	/* Unmask */
	if (mask_tmp)
		unmask(buffer->data, buffer->length, data + header_length);
	
	/* Set output data */
	*fin = fin_tmp;
	*opcode = opcode_tmp;
	*mask = mask_tmp;
	
	/* Remove data from source buffer */
	buffer_remove_data(src_buffer, frame_length);
	
	return FS_READY;

Exit:
	return FS_BUILDING;
}

static void
_wsmsg_process(wsmsg_t *wsmsg)
{
	int fin;
	int opcode;
	int mask;
	enum _frame_status_t fs;
	int res;

	if (wsmsg->message || wsmsg->frame || wsmsg->error)
		return;

	while (1)
	{
		fs = get_frame(
			wsmsg->frame_buffer,
			&fin,
			&opcode,
			&mask,
			wsmsg->frame_data);
		if (fs == FS_BUILDING)
			goto Exit;
		if (fs == FS_READY)
		{
			if ( (opcode == OPCODE_CLOSE) || (opcode == OPCODE_PING) ||
				(opcode == OPCODE_PONG) )
			{
				/* RFC 6455: Control frames themselves MUST NOT be fragmented. */
				if (!fin)
					goto Error;
				wsmsg->frame = 1;
				wsmsg->fin = fin;
				wsmsg->opcode = opcode;
				wsmsg->mask = mask;
				goto Exit;
			}
			if ( wsmsg->message_started && (opcode != OPCODE_CONTINUATION) )
				goto Error;
			if ( !wsmsg->message_started && (opcode == OPCODE_CONTINUATION) )
				goto Error;

			res = buffer_append(
				wsmsg->message_buffer, 
				wsmsg->frame_data->data,
				wsmsg->frame_data->length);
			if (!res)
				goto Error;
			if (!wsmsg->message_started)
			{
				wsmsg->message_started = 1;
				wsmsg->message_opcode = opcode;
			}
			if (fin)
				wsmsg->message = 1; /* We have a full message */
			/* Remove data from frame_data */
			buffer_remove_data(wsmsg->frame_data, wsmsg->frame_data->length);
		}
		if (fs == FS_ERROR)
			goto Error;
	}

Exit:
	return;

Error:
	wsmsg->error = 1;
}

int
wsmsg_add(wsmsg_t *wsmsg, byte *data, size_t length)
{
	int fin;
	int opcode;
	int mask;
	enum _frame_status_t fs;
	int res;
	
	res = buffer_append(wsmsg->frame_buffer, data, length);
	if (!res)
		return 0;
	_wsmsg_process(wsmsg);
	return 1;
}

int
wsmsg_fail(wsmsg_t *wsmsg)
{
	return wsmsg->error;
}

int
wsmsg_has_message(wsmsg_t *wsmsg)
{
	return wsmsg->message;
}

int
wsmsg_get_message(
	wsmsg_t *wsmsg,
	buffer_t *buffer,
	int *opcode)
{
	int res;

	if (!wsmsg->message)
		return 0;
	wsmsg->message = 0;
	wsmsg->message_started = 0;
	res = buffer_move(wsmsg->message_buffer, buffer);
	*opcode = wsmsg->message_opcode;
	_wsmsg_process(wsmsg);
	return res;
}

int wsmsg_has_frame(wsmsg_t *wsmsg)
{
	return wsmsg->frame;
}

int
wsmsg_get_frame(
	wsmsg_t *wsmsg,
	buffer_t *buffer,
	int *fin,
	int *opcode,
	int *mask)
{
	int res;

	if (!wsmsg->frame)
		return 0;
	wsmsg->frame = 0;
	res = buffer_move(wsmsg->frame_data, buffer);
	*fin = wsmsg->fin;
	*opcode = wsmsg->opcode;
	*mask = wsmsg->mask;
	_wsmsg_process(wsmsg);
	return res;
}

