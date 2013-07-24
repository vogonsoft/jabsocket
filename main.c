#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include "parseconfig.h"
#include <event2/util.h>
#include "cmanager.h"
#include "log.h"
#include <signal.h>
#include "jabsocketConfig.h"

struct params_t
{
	char *config; /* config file */
	int nodaemon;   /* don't run as daemon */
};

static void
usage(char *prog)
{
	fprintf(stderr, "Usage: %s <options>\n", prog);
	fprintf(stderr,
"Options:\n"
"  --config, -c <config file>          - Configuration file\n"
"                                        (default is /etc/jabsocket.conf)\n"
"  --nodaemon, -n                      - Don't run as daemon\n"
"                                        (default is running as daemon)\n"
"  --version, -v                       - Display the version\n"
"  --help, -h                          - Help message\n"
	);

	exit(0);
}

static void
version(char *prog)
{
	fprintf(stderr, "%s version %d.%d.%d\n",
		prog,
		jabsocket_VERSION_MAJOR,
		jabsocket_VERSION_MINOR,
		jabsocket_VERSION_PATCH);
	exit(0);
}

static int
parse_params(int argc, char **argv, struct params_t *params)
{
	int c;

	memset( params, 0, sizeof(struct params_t) );
	params->config = "/etc/jabsocket.conf";
	while (1)
	{
		int option_index = 0;
		static struct option long_options[] =
			{
				{"config",   1, 0, 'c'},
				{"nodaemon", 0, 0, 'n'},
				{"version",  0, 0, 'v'},
				{"help",     0, 0, 'h'},
				{0, 0, 0, 0}
			};

		c = getopt_long(argc, argv, "c:hnv",
			long_options, &option_index);
		if (c == -1)
			break;

		switch (c) {
			case 'v':
				version(argv[0]);
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'n':
				params->nodaemon = 1;
				break;
			case 'c':
				params->config = optarg;
				break;
			case '?': /* invalid option */
				usage(argv[0]);
				break;
		}

	}
	return 1;
}

void
signal_callback(evutil_socket_t socket, short what, void *ctx)
{
	LOG(LOG_INFO, "main.c:signal_callback exiting");
}

void
sig_handler(int sig)
{
	LOG(LOG_INFO, "main.c:sig_handler exiting (signal %d)", sig);
	exit(0);
}

int
main(int argc, char **argv)
{
	struct params_t params;
	int res;
	jsconf_t *conf;
	struct evutil_addrinfo hints;
	struct evutil_addrinfo *answer = NULL;
	int err;
	wsserver_t *wsserver;
	struct event_base *base;
	struct event *signal_event;

	if ( !parse_params(argc, argv, &params) )
		usage(argv[0]);

	conf = config_create();
	if (conf == NULL)
	{
		fprintf(stderr, "Error creating configuration reader\n");
		exit(-1);
	}

	res = config_parse(conf, params.config);
	if (!res)
	{
		fprintf(stderr, "Error reading configuration\n");
		exit(-1);
	}
	
	/* Get socket address that we will bind to. */

	/* Build the hints to tell getaddrinfo how to act. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC; /* v4 or v6 is fine. */
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP; /* We want a TCP socket */
	/* Only return addresses we can use. */
	hints.ai_flags = EVUTIL_AI_PASSIVE | EVUTIL_AI_NUMERICSERV |
		EVUTIL_AI_NUMERICHOST;

	/* Look up the hostname. */
	err = evutil_getaddrinfo(conf->cidr, conf->port, &hints, &answer);
	if ( (err != 0) || (answer == NULL) )
	{
		fprintf(stderr, "Error while resolving '%s': %s",
			conf->cidr, evutil_gai_strerror(err));
		exit(-1);
	}

	base = event_base_new();
	if (!base)
	{
		puts("Couldn't open event base");
		exit(-1);
	}

	logopen(conf);

	wsserver = ws_create(base, answer->ai_addr, answer->ai_addrlen);
	if (wsserver == NULL)
	{
		fprintf(stderr, "Error creating WebSocket server\n");
		exit(-1);
	}
	evutil_freeaddrinfo(answer);

	ws_set_cb(wsserver, cm_create, cm_delete, cmanager, NULL);

	/* NOTE: these don't work for some reason. */
	signal_event = event_new(base, SIGINT, EV_SIGNAL|EV_PERSIST, signal_callback, NULL);
	signal_event = event_new(base, SIGTERM, EV_SIGNAL|EV_PERSIST, signal_callback, NULL);

	/* NOTE: I have to set up signal handlers in the standard way - outside
	   libevent, in order to log an exit message. */
	signal(SIGINT, sig_handler);
	signal(SIGTERM, sig_handler);

	event_base_dispatch(base);

	ws_delete(wsserver);
	return 0;
}

