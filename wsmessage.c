#include "wsmessage.h"

wsmsg_t *
wsmsg_create(jsconf_t *conf)
{
	wsmsg_t *wsmsg;
	
	wsmsg = (wsmsg_t *) malloc( sizeof(wsmsg_t) );
	if (wsmsg != NULL)
	{
		memset( wsmsg, 0, sizeof(wsmsg_t) );
	}
	return wsmsg;
}

void
wsmsg_delete(wsmsg_t *wsmsg)
{
	free(wsmsg);
}

