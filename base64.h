#ifndef _BASE64_H_
#define _BASE64_H_

#include <stdlib.h>
#include "util.h"

void base64_encode(
	const unsigned char *data,
    size_t input_length,
    str_t *out_str);

unsigned char *base64_decode(const char *data,
                             size_t input_length,
                             size_t *output_length);
#endif /* _BASE64_H_ */

