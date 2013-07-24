#include "log.h"

void
logopen(jsconf_t *conf)
{
	openlog("jabsocket", LOG_PID, LOG_DAEMON);
	setlogmask( LOG_UPTO(conf->log_level) );

}

void
LOG(int priority, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vsyslog(priority, format, ap);
	va_end(ap);
}

void
VLOG(int priority, const char *format, va_list ap)
{
	vsyslog(priority, format, ap);
}

