#ifndef __CLIENT__H__
#define __CLIENT__H__

#include "kvlist.h"

typedef struct client_s client_t;
struct client_s {
	char *name;
	int (*init)(client_t *client);
	int (*send)(char *email, char *password, kvlist_t **params);
	int (*halt)(client_t *client);
};

#endif /* __CLIENT_H__ */
