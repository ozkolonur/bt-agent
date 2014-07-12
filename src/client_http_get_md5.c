#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "agent.h"
#include "client.h"

#ifndef HTTP_GET_MINING_PLATFORM
#define HTTP_GET_MINING_PLATFORM "http://www.bodyport.org/mining_platform/http_get/?"
#endif


extern void
md5sum (char *result, char *buffer) ;

/* Server response buffers */
struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;
 
  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */ 
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }
 
  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
 
  return realsize;
}

int http_get_md5_init(client_t *client)
{
	printf("init http-get-md5 client\n");
	printf("Using mining platform : %s\n", HTTP_GET_MINING_PLATFORM);
	return 0;
}

#define RES_COMMITTED 0
int http_get_md5_send(char *email, char *password, kvlist_t **result)
{
	CURL *curl;
	CURLcode res;
	struct MemoryStruct response;
	char url[512]= {0};
	char count_url[512]= {0};
  	int mres, ft=0;
	kvlist_t *i = (*result);
	int url_length = 0;
    char *salt = "erdex";
	char secret_key[512] = {0};
	char secret_key_md5[49] = {0};
	
//	printf("%d::sizeof(url)= %d\n", __LINE__,strlen(url));
	/* Construct url string */
	strcat((char *)&url, HTTP_GET_MINING_PLATFORM);
//	printf("%d::sizeof(url)= %d\n", __LINE__,strlen(url));
	
	strcat((char *)&url, "?");
//	printf("%d::sizeof(url)= %d\n", __LINE__,strlen(url));
	for(ft=0;i;i = i->next){
		if (ft == 0)
			ft = 1;
		else
			strcat((char *)&url, "&");
//		printf("%s\n", (i)->key);
		strcat((char *)&url, i->key);
		strcat((char *)&url, "=");
		strcat((char *)&url, i->value);
	}
//	printf("%d::sizeof(url)= %d\n", __LINE__,strlen(url));
    url_length = strlen(url) - strlen(HTTP_GET_MINING_PLATFORM) - 1;

    sprintf(&secret_key, "%s|%04d|%s", salt, url_length, kvlist_get((*result), "mac_addr"));
    printd("secret_key = %s\n", secret_key);
	md5sum(&secret_key_md5, secret_key);
    printd("secret_key_md5 = %s\n", secret_key_md5);

			strcat((char *)&url, "&token=");
 			strcat((char *)&url, secret_key_md5);

//	printf("%s\n", kvlist_get(*result, "hb"));
//	printf("%s\n", SVN_REVISION);

		printd("%s\n",url);

//	return RES_COMMITTED;

		response.memory = malloc(1);  /* will be grown as needed by the realloc above */ 
  		response.size = 0;    /* no data at this point */ 

  		curl_global_init(CURL_GLOBAL_ALL);

    	curl = curl_easy_init();
		if(curl) 
		{    
			curl_easy_setopt(curl, CURLOPT_URL, url); 	   

		  	/* send all data to this function  */ 
  			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
 
  			curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
 
  			curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");

			res = curl_easy_perform(curl);    /* always cleanup */    

			curl_easy_cleanup(curl);
			
			printf("%lu bytes response retrieved\n", (long)response.size);

            if ((long)response.size < 100)
				printf("Server Response\n[%p]=%s\n", response.memory, response.memory);

            if (strstr(response.memory, "OK") != NULL )
			{
				printf("Measurement Committed to server [took x seconds]\n");
				mres = RES_COMMITTED;
			} else if (strstr(response.memory, "FAIL") != NULL)
			{
				printf("Server responded negatively - check login info\n");
				mres = RES_COMMITTED;
			}

				
 			if(response.memory)
    			free(response.memory);
 
  			/* we're done with libcurl, so clean it up */ 
  			curl_global_cleanup();
  
		}
		printf("\n");  
		return mres;

}

int http_get_md5_halt(client_t *client)
{
	return 0;
}

client_t http_get_md5 = {
  .name = "http_get_md5",
  .init = http_get_md5_init,
  .send = http_get_md5_send,
  .halt = http_get_md5_halt,
};
