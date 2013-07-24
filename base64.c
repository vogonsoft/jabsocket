/*
 * base64 implementation copied from StackOverflow:
 * http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
 * Implemented by ryyst (http://stackoverflow.com/users/282635/ryyst)
 */
#include "base64.h"
#include <stdint.h>
#include <stdlib.h>

static void build_decoding_table();

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;
static int mod_table[] = {0, 2, 1};

void
base64_encode(
	const unsigned char *data,
    size_t input_length,
    str_t *out_str)
{
    int i, j;
    size_t output_length;
    char *str;

	str_set_string(out_str, "");
	j = mod_table[input_length % 3];
    for (i = 0; i < input_length; ) {
        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		str_append_char(out_str, encoding_table[(triple >> 3 * 6) & 0x3F]);
		str_append_char(out_str, encoding_table[(triple >> 2 * 6) & 0x3F]);
		str_append_char(out_str, encoding_table[(triple >> 1 * 6) & 0x3F]);
		str_append_char(out_str, encoding_table[(triple >> 0 * 6) & 0x3F]);
    }

	output_length = str_get_length(out_str);
	str = str_get_string(out_str);
	if (j > 0)
		str[output_length - 1] = '=';
	if (j > 1)
		str[output_length - 2] = '=';
}

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length) {
	int i, j;

    if (decoding_table == NULL) build_decoding_table();

    if (input_length % 4 != 0) return NULL;

    *output_length = input_length / 4 * 3;
    if (data[input_length - 1] == '=') (*output_length)--;
    if (data[input_length - 2] == '=') (*output_length)--;

    unsigned char *decoded_data = malloc(*output_length);
    if (decoded_data == NULL) return NULL;

    for (i = 0, j = 0; i < input_length; ) {

        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[(int) data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[(int) data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[(int) data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[(int) data[i++]];

        uint32_t triple = (sextet_a << 3 * 6)
        + (sextet_b << 2 * 6)
        + (sextet_c << 1 * 6)
        + (sextet_d << 0 * 6);

        if (j < *output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return decoded_data;
}


static void build_decoding_table() {
	int i;
    decoding_table = malloc(256);

    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


void base64_cleanup() {
    free(decoding_table);
}

