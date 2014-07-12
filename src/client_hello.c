#include <stdio.h>
#include "client.h"
#include "kvlist.h"

int hello_init(client_t *client)
{
	printf("%s\n", __func__);
	return 0;
}

int hello_send(char *email, char *password, kvlist_t **params)
{
	printf("%s\n", __func__);
	printf("Parameters to send:\n");
	kvlist_print(*params);
	printf("done\n");
	return 0;
}

int hello_halt(client_t *client)
{
	printf("%s\n", __func__);
	return 0;
}

client_t hello = {
  .name = "client_hello",
  .init = hello_init,
  .send = hello_send,
  .halt = hello_halt,
};
