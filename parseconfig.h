#ifndef _PARSECONFIG_H_
#define _PARSECONFIG_H_

#include <yaml.h>
#include <syslog.h>

#define YAML_MAX_VALUE_SIZE 128
#define YAML_MAX_STACK_SIZE 16

typedef struct _origin_t origin_t;
struct _origin_t
{
	char *url; /* regular expression for an origin URL */
	origin_t *next;
};

typedef struct _jsconf_t
{
	char *port;
	origin_t *origin_list;
	char *cidr;
	char *host;
	char *resource;
	int log_level;
	int max_message_size;
	int max_frame_size;
} jsconf_t;

jsconf_t *config_create();
void config_delete(jsconf_t *conf);

int config_parse(jsconf_t *conf, const char *file);
int config_check_origin(jsconf_t *conf, const char *origin);

#endif /* _PARSECONFIG_H_ */

