#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/fcntl.h>
#include <malloc.h>
#include "driver.h"
#include "kvlist.h"

#define BUFFER_SIZE 1024
extern void device_callback(void *data);

unsigned char myRand()
{
    struct timeval seed;
    gettimeofday(&seed, NULL);
    srand(seed.tv_usec); 
    return rand();
}

int driver_hello_init(device_t *device)
{
	device->status = DEV_INITALIZED;
	device->after_conn_timeout = 45;
	device->after_meas_timeout = 30;
	device->delete_after_n_sec = 0;
	device->try_after_n_sec = 0;
	return 0;
}

static int extract_measurement(unsigned char buffer[BUFFER_SIZE], device_t *device)
{
    char tmp[8] = {0};
	printd("%s\n", __func__);

	sprintf(&tmp, "%d", 120 + myRand()%10);
	kvlist_add(&(device->result), "sys", (char *)&tmp);
	sprintf(&tmp, "%d", 80 + myRand()%10);
	kvlist_add(&(device->result), "dia", (char *)&tmp);
	sprintf(&tmp, "%d", 80 + myRand()%10);
	kvlist_add(&(device->result), "hb", (char *)&tmp);
	kvlist_add(&(device->result), "note", "collected");
}

int driver_hello_listen(device_t *device)
{
	printd("%s\n", __func__);
	kvlist_add(&(device->result), "mac_addr", device->dev_id);
	extract_measurement(NULL, device);
//	printd("mac_addr : %s\n", kvlist_get((device->result), "mac_addr"));
//	kvlist_print(device->result);
	printf("-------------------------------------\n");
	kvlist_print(device->result);
	printf("-------------------------------------\n");
	return DEV_MEAS_VALID;
}

int driver_hello_halt(device_t *dev)
{
	printd("%s\n", __func__);
	dev->status = DEV_HALTED;
	return 0;
}

driver_t driver_hello = {
  .name = "driver_hello",
  .init = driver_hello_init,
  .listen = driver_hello_listen,
  .halt = driver_hello_halt,
};


