#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>
#include <malloc.h>
#include <stdlib.h>
#include "driver.h"
#include "client.h"
#include "hexdump.h"

#define BUFFER_SIZE 32

extern void device_callback(void *data);

static int extract_measurement(char buffer[BUFFER_SIZE], int *weight, int *frac)
{
	char tmp_weight[8];
	char tmp_frac[8];

	int i = 0;
	for(i=0; i<BUFFER_SIZE; i++)
		if(buffer[i] == 0x81)
			if(buffer[i+1] == 0x00)
					if(buffer[i+4] == 0xaa)
					{
						sprintf((char *)&tmp_weight, "%x", (int)buffer[i+2]);
						sprintf((char *)&tmp_frac, "%x", (int)buffer[i+3]);
						*weight = atoi(tmp_weight);
						*frac = atoi(tmp_frac);
						printf("Your weight is %d,%d\n", *weight, *frac);
						return DEV_MEAS_VALID;
					}
	
	return DEV_MEAS_INVALID;
}

int device_avita_bs_init(device_t *device)
{
	device->status = DEV_INITALIZED;
	printf("%s\n", __func__);
	return 0;
}

#define QUEUE_SIZE 10
int device_avita_bs_listen(device_t *device, kvlist_t **result, char *dev_id, int timeout)
{

    struct sockaddr_rc addr = { 0 };
    int s, stat, i=0, t=0, locked=0;
	struct pollfd fds;
	char tmp[32] = { 0 };
    char buf[BUFFER_SIZE] = { 0 };
	int numbytes;
	int weights[QUEUE_SIZE], fracs[QUEUE_SIZE];
	time_t con_time, cur_time, latest_data_time;;

    // allocate a socket
	printf("target dev_id : %s \n", dev_id);
    s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

    // set the connection parameters (who to connect to)
    addr.rc_family = AF_BLUETOOTH;
    addr.rc_channel = (uint8_t) 1;
    str2ba( dev_id, &addr.rc_bdaddr );

	fds.fd = s;
	fds.events = POLLIN | POLLPRI;

	while(TRUE)
	{
		if (device->status == DEV_INITALIZED)
		{
			usleep(4000000);
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
			if ( poll(&fds, 1, 10) > 0)
			{
				numbytes = recv(s, &buf, 5, 0);
				printf("received %d bytes.\n", numbytes);
				fflush(stdout);

				// 81 00 79 80 aa ...
				if (extract_measurement(buf, &weights[i], &fracs[i]) == DEV_MEAS_INVALID)
					continue;
				printf("got meas [%d] weight=%d fracs=%d\n", i, weights[i], fracs[i]);

				i++;
				if (i>(QUEUE_SIZE-1))
					{ i=0; }

				// assume it is locked
				locked = 1;
				for(t=0; t<(QUEUE_SIZE); t++)
				{
					if ((weights[0] != weights[t]) || (fracs[0] != fracs[t]))
						locked = 0;
					printf("==%d==weight is %d %d \n", t, weights[t], fracs[t]);
				}
				latest_data_time = time(NULL);

				if ((locked == 1) && (weights[0] != 0))
				{
					sprintf((char *)&tmp, "%d.%d", weights[0], fracs[0]);
					kvlist_add(result, "weight_kg", tmp);
					device->status = DEV_MEAS_VALID;
					printf("Your weight is locked at %d.%d KG\n", weights[0], fracs[0]);
					return DEV_MEAS_VALID;
				}
				
				continue;
			}
		} else /* !DEV_CONNECTED */				
		{
			cur_time = time(NULL);
			if (cur_time - con_time > timeout)
			{
				if (latest_data_time - cur_time < 10)
					continue;
				close(s);
				device->status = DEV_CONN_TIMEOUT;
				return DEV_MEAS_INVALID;
			}
		}

	}

	return DEV_MEAS_INVALID;
}

int device_avita_bs_halt(device_t *device)
{
	device->status = DEV_HALTED;
	printf("%s\n", __func__);
	return 0;
}

driver_t driver_avita_bs = {
  .name = "device_avita_bs",
  .init = device_avita_bs_init,
  .listen = device_avita_bs_listen,
  .halt = device_avita_bs_halt,
};


