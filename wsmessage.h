#ifndef _WSMESSAGE_H_
#define _WSMESSAGE_H_

#include "parseconfig.h"

typedef struct _wsmsg_t_
{

} wsmsg_t;

wsmsg_t *wsmsg_create(jsconf_t *conf);
void wsmsg_delete(wsmsg_t *wsmsg);

#endif /* _WSMESSAGE_H_ */

