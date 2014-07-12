#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>

#include "client.h"

#ifndef HTTP_GET_MINING_PLATFORM
#define HTTP_GET_MINING_PLATFORM "127.0.0.1"
#endif

#define SERVER_PORT 3324
#define MAX_BUFSIZE	200

int tcp_json_init(client_t *client)
{
	printf("%s\n", __func__);
	return 0;
}

int tcp_json_send(char *email, char *password, kvlist_t **result)
{
    int isockfd=0;
    struct sockaddr_in stServerAddr;
    int i=0, ft=0;
    int true = 1;
    char cbuf[MAX_BUFSIZE]={0};
    char tmp[64];
	kvlist_t *j = (*result);

    printf("Server Started\n");
    if ((isockfd = socket(PF_INET, SOCK_STREAM, 0)) == -1)
    {
	perror("Server socket create");
	exit(1);
    }
    
    if (setsockopt(isockfd, SOL_SOCKET, SO_REUSEADDR, &true, sizeof(int)) == -1)
    {
	perror("Server socket option");
	exit(1);
    }
    
    stServerAddr.sin_family = PF_INET;
    stServerAddr.sin_port = htons(SERVER_PORT);
    stServerAddr.sin_addr.s_addr = INADDR_ANY;
    memset(stServerAddr.sin_zero, '\0', sizeof(stServerAddr.sin_zero));

//    if (bind(isockfd, (struct sockaddr *)&stServerAddr, sizeof(stServerAddr)) == -1)
//    {
//	perror("Server socket bind");
//	exit(1);
//    }

//		if (ft == 0)
//			ft = 1;
//		else
//			strcat((char *)&url, "&");

	/* Construct string */
	strcat((char *)&cbuf, "{");
	sprintf((char *)&tmp, "\"email\":\"%s\", \"password\"=\"%s\",", email, password);
	strcat((char *)&cbuf, tmp);

	for(;j->next;j = j->next){
		if (ft == 0)
			ft = 1;
		else
			strcat((char *)&cbuf, ",");
		strcat((char *)&cbuf, "\"");
		strcat((char *)&cbuf, j->key);
		strcat((char *)&cbuf, "\":\"");
		strcat((char *)&cbuf, j->value);
		strcat((char *)&cbuf, "\"");
	}
	strcat((char *)&cbuf, "}\n");
	printf("%s", cbuf);

    if((i = write(isockfd, cbuf, MAX_BUFSIZE)) < 0)
	perror("writing to socket");

    printf("here2\n");
    printf("i=%d\n", i);
    close(isockfd);


    return 0;


}

int tcp_json_halt(client_t *client)
{
	printf("%s\n", __func__);
	return 0;
}

client_t tcp_json = {
  .name = "tcp_json",
  .init = tcp_json_init,
  .send = tcp_json_send,
  .halt = tcp_json_halt,
};
