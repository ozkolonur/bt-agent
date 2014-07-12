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
#include "kvlist.h"
#include "hexdump.h"

#define BUFFER_SIZE 1024

extern void device_callback(void *data);

static int extract_measurement(unsigned char buffer[BUFFER_SIZE], int numbytes, device_t *device)
{
	int i = 0;
	char tmp[32];
	for(i=0; i<numbytes; i++)
		if(buffer[i] == 0x01)
			if(buffer[i+1] == 0x02)
				if(buffer[i+2] == 0xff)
					if(buffer[i+6] == 0x01)
						if(buffer[i+7] == 0x03)
						{
							printd("\nbt-agent: Measurement Received\n");
							sprintf((char *)&tmp, "%d", (int)buffer[i+3]);
							kvlist_add(&(device->result), "sys", (char *)&tmp);
							sprintf((char *)&tmp, "%d", (int)buffer[i+4]);
							kvlist_add(&(device->result), "dia", (char *)&tmp);
							sprintf((char *)&tmp, "%d", (int)buffer[i+5]);
							kvlist_add(&(device->result), "hb", (char *)&tmp);
							kvlist_print(device->result);
							return DEV_MEAS_VALID;
						}
	return DEV_MEAS_INVALID;
}


int driver_mgh_init(device_t *device)
{
	device->status = DEV_INITALIZED;
	device->after_conn_timeout = 45;
	device->after_meas_timeout = 30;
	device->result = NULL;
	printf("%s\n", __func__);
	return 0;
}

int driver_mgh_listen(device_t *device)
{

    struct sockaddr_rc addr = { 0 };
    int s, stat;
	struct pollfd fds;
	
    unsigned char buf[BUFFER_SIZE] = { 0 };
	int numbytes;
//	unsigned char buf2[] = {0x80, 0x01, 0xFE, 0x0A, 0x81, 0xF4};
	unsigned char buf2[] = {0x80, 0x01, 0xFE, 0x00, 0x81, 0xFE};
	int tmp =0;
	time_t con_time, cur_time, latest_data_time;;

	kvlist_add(&(device->result), "mac_addr", device->dev_id);

    // allocate a socket
	printf("target dev_id : %s \n", device->dev_id);
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( device->dev_id, &addr.rc_bdaddr );

	fds.fd = s;
	fds.events = POLLIN | POLLPRI;

	while(TRUE)
	{
		//printf("device->status=%d socket=%d\n", device->status, s);
		if (device->status == DEV_INITALIZED)
		{
			usleep(500000);
	pthread_mutex_lock(&hci_inquiry_lock);
			if ((stat = connect(s, (struct sockaddr *)&addr, sizeof(addr)) != -1))
			{
				printf("connected to device [stat=%d]\n", stat);
				con_time = time(NULL);
				latest_data_time = time(NULL);
				usleep(1000000);
				device->status = DEV_CONNECTED;
			}
	pthread_mutex_unlock(&hci_inquiry_lock);
		}
		
		if (device->status == DEV_CONNECTED)
		{
			printf("Size of buf2 is %d\n", sizeof(buf2));
			tmp = send(s, (unsigned char *)&buf, sizeof(buf2), 0);
			printf("Send %d bytes\n", tmp);		
				numbytes = recv(s, (unsigned char *)&buf, BUFFER_SIZE, 0);
				printf("read %d bytes\n", numbytes);

			if ( poll(&fds, 1, 100) > 0)
			{
				memset(buf, 0, BUFFER_SIZE);
				numbytes = recv(s, (unsigned char *)&buf, BUFFER_SIZE, 0);
				printf(".");
				fflush(stdout);
				latest_data_time = time(NULL);
				if (numbytes == -1)
				{
					close(s);
					memset(buf, 0, BUFFER_SIZE);
					device->status = DEV_CONN_TIMEOUT;
					return DEV_MEAS_INVALID;
				}
					
				if (extract_measurement(buf, numbytes, device) == DEV_MEAS_VALID)
				{
					close(s);
					memset(buf, 0, BUFFER_SIZE);
					device->status = DEV_MEAS_VALID;
					return DEV_MEAS_VALID;
				}

			}
			else
			{

		    	cur_time = time(NULL);
				if (cur_time - con_time > device->after_conn_timeout)
				{
					if (cur_time - latest_data_time < 10)
						continue;
					close(s);
					memset(buf, 0, BUFFER_SIZE);
					device->status = DEV_CONN_TIMEOUT;
					return DEV_MEAS_INVALID;
				}

			}
		} else /* !DEV_CONNECTED */				
		{
			cur_time = time(NULL);
			if (cur_time - con_time > device->after_conn_timeout)
			{
				if (cur_time - latest_data_time < 10)
					continue;
				close(s);
				device->status = DEV_CONN_TIMEOUT;
				return DEV_MEAS_INVALID;
			}
		}

	}

	return DEV_MEAS_INVALID;
}

int driver_mgh_halt(device_t *device)
{
	device->status = DEV_HALTED;
	printf("%s\n", __func__);
	return 0;
}

driver_t driver_mgh = {
  .name = "driver_mgh",
  .init = driver_mgh_init,
  .listen = driver_mgh_listen,
  .halt = driver_mgh_halt,
};


