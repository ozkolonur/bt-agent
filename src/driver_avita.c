#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/poll.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <malloc.h>
#include "driver.h"
#include "client.h"
#include "hexdump.h"

#define BUFFER_SIZE 1024

extern pthread_mutex_t hci_inquiry_lock;

extern void device_callback(void *data);

static int set_pin(char *pin)
{
	char cmd[64] = { 0 };
	int res = 0;
	sprintf(&cmd, "echo \"#!/bin/sh\n\necho \"PIN:%s\"\" > /var/givepin\n", pin);
	res = system(cmd);
	system("chmod +x /var/givepin");
	if (res != -1)
		printd("set_pin successful");

	return 0;
}

//static int extract_measurement(unsigned char buffer[BUFFER_SIZE], kvlist_t **result)
static int extract_measurement(unsigned char buffer[BUFFER_SIZE], device_t *device)
{
	int i = 0;
	char tmp[32];
	int sys=0,dia=0,hb=0;
	for(i=0; i<BUFFER_SIZE; i++)
		if(buffer[i] == 0xa5)
			if(buffer[i+1] == 0xf5)
				if(buffer[i+2] == 0x00)
					if(buffer[i+4] == 0xaa)
					{
						sys = (int)buffer[i+3];
						break;
					}

	for(i=0; i<BUFFER_SIZE; i++)
		if(buffer[i] == 0xa5)
			if(buffer[i+1] == 0xf6)
				if(buffer[i+2] == 0x00)
					if(buffer[i+4] == 0xaa)
					{
						dia = (int)buffer[i+3];
						break;
					}

	for(i=0; i<BUFFER_SIZE; i++)
		if(buffer[i] == 0xa5)
			if(buffer[i+1] == 0xf4)
				if(buffer[i+2] == 0x00)
					if(buffer[i+4] == 0xaa)
					{
						hb = (int)buffer[i+3];

							sprintf((char *)&tmp, "%d", sys);
							kvlist_add(&(device->result), "sys", (char *)&tmp);

							sprintf((char *)&tmp, "%d", dia);
							kvlist_add(&(device->result), "dia", (char *)&tmp);

							sprintf((char *)&tmp, "%d", hb);
							kvlist_add(&(device->result), "hb", (char *)&tmp);

						printd("\nbt-agent: sys = %d dia = %d hb = %d\n", sys, dia, hb);
						return DEV_MEAS_VALID;
					}
	
	return DEV_MEAS_INVALID;
}

int driver_avita_init(device_t *device)
{
	int res;
	printf("%s\n", __func__);
	pthread_mutex_lock(&hci_inquiry_lock);
	set_pin("0000");

	device->status = DEV_INITALIZED;
	device->after_conn_timeout = 45;
	device->after_meas_timeout = 30;
	device->result = NULL;
//	res = system("hcitool cc 00:1C:97:33:00:8C");
//	printf("hcitool cc 00:1C:97:33:00:8C returned %d\n", res);
	
	pthread_mutex_unlock(&hci_inquiry_lock);

	return 0;
}

//int device_avita_listen(device_t *device, kvlist_t **result, char *dev_id, int timeout)
int driver_avita_listen(device_t *device)
{

    struct sockaddr_rc addr = { 0 };
    int s, stat;
	struct pollfd fds;
    unsigned char buf[BUFFER_SIZE] = { 0 };
	char *bufptr = (char *)&buf;
	int len=0, numbytes=0;

	time_t con_time, cur_time, latest_data_time;;
	printf("%s\n", __func__);

	kvlist_add(&(device->result), "mac_addr", device->dev_id);
	kvlist_add(&(device->result), "bpm_mac", device->dev_id);

    // allocate a socket
	printf("target dev_id : %s \n", device->dev_id);
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba(device->dev_id, &addr.rc_bdaddr );

	fds.fd = s;
	fds.events = POLLIN | POLLPRI;
	latest_data_time = time(NULL);
TRACE;

	while(TRUE)
	{
		if (device->status == DEV_INITALIZED)
		{
TRACE;
			usleep(5000);
	pthread_mutex_lock(&hci_inquiry_lock);
			if ((stat = connect(s, (struct sockaddr *)&addr, sizeof(addr)) != -1))
			{
TRACE;
				printf("connected to device [stat=%d]\n", stat);
				con_time = time(NULL);
				latest_data_time = time(NULL);
				usleep(500000);
				device->status = DEV_CONNECTED;
			}
	pthread_mutex_unlock(&hci_inquiry_lock);
		}
		
		if (device->status == DEV_CONNECTED)
		{
			if ( poll(&fds, 1, 10) > 0)
			{
TRACE;
				numbytes = recv(s, bufptr, BUFFER_SIZE, 0);
				printf("received %d bytes.\n", numbytes);
				fflush(stdout);
				bufptr += numbytes;
				len += numbytes;
				latest_data_time = time(NULL);
TRACE;
				if (numbytes == -1)
				{
TRACE;
					close(s);
					memset(buf, 0, BUFFER_SIZE);
TRACE;
					device->status = DEV_CONN_TIMEOUT;
					return DEV_MEAS_INVALID;
				}

				if ( len > (BUFFER_SIZE - 1))
				{
					printf("Fatal Error, Buffer area exceeded [len=%d]\n", len);
					exit(EXIT_FAILURE);
				}
			}
			else
			{
				if (len > 0)
					if (extract_measurement(buf, device) == DEV_MEAS_VALID)
					{
TRACE;
						close(s);
						memset(buf, 0, BUFFER_SIZE);
						bufptr = buf;
						device->status = DEV_MEAS_VALID;
						return DEV_MEAS_VALID;
					}
		    	cur_time = time(NULL);
				if (cur_time - con_time > device->after_conn_timeout)
				{
TRACE;
					if (latest_data_time - cur_time > 10)
						continue;
					close(s);
					memset(buf, 0, BUFFER_SIZE);
					bufptr = (char *)&buf;
					device->status = DEV_CONN_TIMEOUT;
					return DEV_MEAS_INVALID;
				}

			}
		} else /* !DEV_CONNECTED */				
		{
TRACE;
			cur_time = time(NULL);
			if (cur_time - con_time > device->after_conn_timeout)
			{
TRACE;
				if (latest_data_time - cur_time  > 10)
					continue;
				close(s);
				device->status = DEV_CONN_TIMEOUT;
				return DEV_MEAS_INVALID;
			}
		}

	}

	return DEV_MEAS_INVALID;
}

int driver_avita_halt(device_t *device)
{
	device->status = DEV_HALTED;
	printf("%s\n", __func__);
	return 0;
}

driver_t driver_avita = {
  .name = "driver_avita",
  .init = driver_avita_init,
  .listen = driver_avita_listen,
  .halt = driver_avita_halt,
};


