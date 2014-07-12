#include <stdio.h>   /* for printf */
#include <stdlib.h>  /* for malloc */
#include <string.h>
#include "driver.h"
 

#ifdef DRIVER_HELLO
extern driver_t driver_hello;
#endif

#ifdef DRIVER_RFCOMM_MYTECH
extern driver_t driver_rfcomm_mytech;
#endif

#ifdef DRIVER_RFSOCK_MYTECH
extern driver_t driver_rfsock_mytech;
#endif

#ifdef DRIVER_AVITA
extern driver_t driver_avita;
#endif

#ifdef DRIVER_AVITA_BS
extern driver_t driver_avita_bs;
#endif

#ifdef DRIVER_AVITA_BS
extern driver_t driver_mgh;
#endif


static driver_t *drivers[] = {
#ifdef DRIVER_HELLO
	&driver_hello,
#endif
#ifdef DRIVER_RFCOMM_MYTECH
	&driver_rfcomm_mytech,
#endif
#ifdef DRIVER_RFSOCK_MYTECH
	&driver_rfsock_mytech,
#endif
#ifdef DRIVER_AVITA
	&driver_avita,
#endif
#ifdef DRIVER_AVITA_BS
	&driver_avita_bs,
#endif
#ifdef DRIVER_MGH
	&driver_mgh,
#endif
	NULL,
};

device_t *device_list_add(device_t **p, char *dev_id, int type) {
    /* some compilers don't require a cast of return value for malloc */
    device_t *n = (device_t *)malloc(sizeof(device_t));
    if (n == NULL)
        return NULL;
    n->next = *p;                                                                            
    *p = n;
    n->dev_id = strdup(dev_id);
	n->type = type;
    return n;
}
 
void device_list_remove(device_t **p) { /* remove head */
    if (*p != NULL) {
        device_t *n = *p;
        *p = (*p)->next;

TRACE;
		if (n->name)
			free(n->name);
TRACE;
//		TODO fix here: causes SegFault
//		if (n->driver)
//			free(n->driver);
TRACE;
//		if (n->dev_id)
//			free(n->dev_id);
TRACE;

        free(n);
    }
}
 
device_t **device_list_search(device_t **n, char *dev_id) {
    while (*n != NULL) {
        if (strcmp((*n)->dev_id, dev_id) == 0) {
            return n;
        }
        n = &(*n)->next;
    }
    return NULL;
}
 
void device_list_print(device_t *n) {
        printf("\n\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

    if (n == NULL) {
        printf("list is empty\n");
    }
    while (n != NULL) {
        printf("print %p %s\n", n, n->dev_id);
        n = n->next;
    }
        printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n\n\n");
}


int attach_drivers(device_t *dev)
{
	driver_t **driver;
	char *name = NULL;

	if(strncmp(dev->dev_id, "AA:AA:AA", 8) == 0)
	{
		name = strdup("driver_hello");
		printd("Using driver: %s\n", name);
	}

	if(strncmp(dev->dev_id, "00:1C:97", 8) == 0)
	{
		name = strdup("driver_rfsock_mytech");
		printd("Using driver: %s\n", name);
	}


	if(strncmp(dev->dev_id, "00:80:25", 8) == 0)
	{
		name = strdup("driver_avita");
		printd("Using driver: %s\n", name);
	}


	if(strncmp(dev->dev_id, "00:13:7B", 8) == 0)
	{
		name = strdup("driver_mgh");
		printd("Using driver: %s\n", name);
	}

	if (name == NULL)
	{
		dev->status = DEV_ALIEN;
		return SUCCESS;
	}

	for(driver = drivers; *driver; driver++)
	{
		if (strcmp(name, (*driver)->name) == 0)
		{
			printd("Setting current driver -> %s\n", (*driver)->name);
			dev->driver = strdup((*driver)->name);
			dev->init = (*driver)->init;
			dev->listen = (*driver)->listen;
			dev->halt = (*driver)->halt;
			return SUCCESS;
		}
	}
	return ERROR;
}



