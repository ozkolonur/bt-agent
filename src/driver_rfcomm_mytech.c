#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/signal.h>
#include <curl/curl.h>
#include <time.h>
#include "driver.h"


#define BUFFER_SIZE 16 * 1024

#define ST_SCANNING		0
#define ST_CONNECTING	1
#define ST_CONNECTED	2
#define RES_INVALID		0
#define RES_VALID		1
#define RES_COMMITTED	2
#define TRUE 			1
#define FALSE 			0

struct termios options;
int init_serial(char *device);
int data_arrived = 0;
int state = 0;

extern void device_callback(void *data);

/*
*  Returns fd
*/
int spp_open(char *device)
{
    int fd_new=-1;
    fd_new = open(device, O_RDONLY | O_NOCTTY | O_NONBLOCK);

    if(fd_new < 0)
		return fd_new;

	fcntl(fd_new, F_SETFL, FASYNC);
    tcgetattr(fd_new, &options);
    options.c_cflag     |= (CLOCAL | CREAD);
    options.c_lflag     &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag     &= ~OPOST;
    options.c_cc[VMIN]  = 0;
    options.c_cc[VTIME] = 150;
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CRTSCTS;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    tcsetattr(fd_new, TCSANOW, &options);

    return fd_new;
}


int spp_close(int fd)
{
	close(fd);
	fd = -1;
	return 0;
}

void signal_handler_IO(int status)
{        
	printf(".");
	fflush(stdout);
    data_arrived = TRUE;
}


int extract_measurement(char buffer[BUFFER_SIZE], kvlist_t **result)
{
	int i = 0;
	char tmp[32];
	for(i=0; i<BUFFER_SIZE; i++)
		if(buffer[i] == 1)
			if(buffer[i+1] == 2)
				if(buffer[i+2] == 255)
					if(buffer[i+6] == 1)
						if(buffer[i+7] == 3)
						{
							printf("\nbt-agent: Measurement Received\n");
							sprintf((char *)&tmp, "%d", (int)buffer[i+3]);
							kvlist_add(result, "sys", (char *)&tmp);

							sprintf((char *)&tmp, "%d", (int)buffer[i+4]);
							kvlist_add(result, "dia", (char *)&tmp);

							sprintf((char *)&tmp, "%d", (int)buffer[i+5]);
							kvlist_add(result, "hb", (char *)&tmp);

							return DEV_MEAS_VALID;
						}
	return DEV_MEAS_INVALID;
}

int rfcomm_mytech_init(device_t *device)
{
	printf("%s\n", __func__);
	return DEV_INITALIZED;
}

int rfcomm_mytech_listen(device_t *device, kvlist_t **result, char *dev_id, int timeout)
{
	char buffer[BUFFER_SIZE];
	char *bufptr = buffer;
	int nbytes;
	struct sigaction saio;           /* definition of signal action */
	int fd=0;
	time_t con_time, cur_time;

    /* install the signal handler before making the device asynchronous */        
	saio.sa_handler = signal_handler_IO;        
	//saio.sa_mask = 0;        
	saio.sa_flags = 0;        
	saio.sa_restorer = NULL;        
	sigaction(SIGIO,&saio,NULL);

    /* allow the process to receive SIGIO */        
	fcntl(fd, F_SETOWN, getpid());

	/* HCI Initialization should be done here*/
	
	printf("bt-agent: Connected to BT Device [%s]\n", dev_id);

    while(1)
    {
        state = ST_CONNECTING;
   	 	if( (fd = spp_open("/dev/rfcomm0")) < 0)
    	{
			if (errno == 0x70) /* Host is down */
			{
				printf("bt-agent: BT Host is down\n");
				continue;
			}
			else
			{
				printf("bt-agent: Unknown errno = %x\n", errno);
			}
    	} 
		else
		{
			state = ST_CONNECTED;
			printf("bt-agent: Connected to BT Device [%s]\n", dev_id);
        }
        data_arrived = FALSE;
		con_time = time(NULL);
		while(state == ST_CONNECTED)
		{
			if (data_arrived == TRUE)
			{
				nbytes = read(fd,bufptr,BUFFER_SIZE);
				bufptr += nbytes;
				data_arrived = FALSE;
				con_time = time(NULL);
				if (extract_measurement(buffer, result) == DEV_MEAS_VALID)
				
				{
					return DEV_MEAS_VALID;

					/* do something here */
					spp_close(fd);
    				memset(buffer, 0, BUFFER_SIZE);
					bufptr = buffer;
					sleep(4);
					break;
				}
			}
		    cur_time = time(NULL);
			if(cur_time - con_time > 30)
			{
				spp_close(fd);
				memset(buffer, 0, BUFFER_SIZE);
				bufptr = buffer;
				state = ST_CONNECTING;
				printf("bt-agent: Disconnected from device [%s]\n", dev_id);
				break;
			}
				
			/* if no event in specified time, check connection */
			usleep(100000);
		}
		sleep(1);
    }

    close(fd);

	return DEV_MEAS_INVALID;
}

int rfcomm_mytech_halt(device_t *device)
{
	printf("%s\n", __func__);
	return DEV_HALTED;
}

driver_t driver_rfcomm_mytech = {
  .name = "driver_rfcomm_mytech",
  .init = rfcomm_mytech_init,
  .listen = rfcomm_mytech_listen,
  .halt = rfcomm_mytech_halt,
};


