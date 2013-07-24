#ifndef _LOG_H_
#define _LOG_H_

#include "parseconfig.h"
#include <stdarg.h>

void logopen(jsconf_t *conf);
void LOG(int priority, const char *format, ...);
void VLOG(int priority, const char *format, va_list ap);

#endif /* _LOG_H_ */

