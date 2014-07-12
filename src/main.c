/*

 CHANGELOG
 ---------
 12/12/2010 - Initial Version
 19/12/2010 - Try until success
 19/06/2011	- Add modular clients
 11/07/2011 - Add modular devices
 24/07/2011 - Catch events using HCI lib  
 27/07/2011	- Add scan_nearby function
 30/07/2011 - kvlist added, result.h removed	
 04/08/2011 - Add threads for concurrent detection	
 18/08/2011 - merge cur_dev_t and device_t
 18/08/2011 - More object based system
 27/09/2011 - Major clean-up, beta version ready
 07/10/2011 - threads clean-up
 15/10/2011	- logger module added.

*/


#include <stdio.h>
#include <string.h>
#include <unistd.h>
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

#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>

#include "agent.h"
#include "client.h"
#include "driver.h"
#include "logger.h"

#ifdef CLIENT_HELLO
extern client_t hello;
#endif

#ifdef CLIENT_HTTP_GET
extern client_t http_get;
#endif

#ifdef CLIENT_HTTP_GET_MD5
extern client_t http_get_md5;
#endif

#ifdef CLIENT_TCP_JSON
extern client_t tcp_json;
#endif

static client_t *clients[] = {
#ifdef CLIENT_HELLO
	&hello,
#endif
#ifdef CLIENT_HTTP_GET
	&http_get,
#endif
#ifdef CLIENT_HTTP_GET_MD5
	&http_get_md5,
#endif
#ifdef CLIENT_TCP_JSON
	&tcp_json,
#endif
	NULL,
};

pthread_mutex_t hci_inquiry_lock;
pthread_mutex_t device_list_lock;

char *email, *password=NULL, *client_name, *device_name, *test_option=NULL;

device_t *device_list = NULL;
int next_scan_nearby_delay = 100000;


client_t *get_client(char *name)
{
  	client_t **client;
	for(client = clients; *client; client++)
	{
		if (strcmp(name, (*client)->name) == 0)
		{
			printd("Set current client -> %s\n", (*client)->name);
			return *client;
		}
	}
	return NULL;
}

void device_callback(void **data)
{
    printf("device callback received\n");
	return;  
}

void print_usage()
{
	printf("This program is used for collecting data from bluetooth or usb devices\n");
	printf("Version:svn%s\n", SVN_REVISION);
	printf("Usage:\n");
	printf("agent -c <client_name> [-d device_name] [-e <email, or username>] [-p<password>]\n");
    printf("possible drivers: driver_hello, driver_rfsock_mytech...\n");
    printf("possible clients: client_hello, device_http_get...\n");
	return;  
}

int open_socket(int dev, unsigned long flags)
{
	struct sockaddr_hci addr;
	struct hci_filter flt;
	struct hci_dev_info di;
	int sk, dd, opt;

	if (dev != HCI_DEV_NONE) {
		dd = hci_open_dev(dev);
		if (dd < 0) {
			perror("Can't open device");
			return -1;
		}

		if (hci_devinfo(dev, &di) < 0) {
			perror("Can't get device info");
			return -1;
		}

		opt = hci_test_bit(HCI_RAW, &di.flags);
		if (ioctl(dd, HCISETRAW, opt) < 0) {
			if (errno == EACCES) {
				perror("Can't access device");
				return -1;
			}
		}

		hci_close_dev(dd);
	}

	/* Create HCI socket */
	sk = socket(AF_BLUETOOTH, SOCK_RAW, BTPROTO_HCI);
	if (sk < 0) {
		perror("Can't create raw socket");
		return -1;
	}

	opt = 1;
	if (setsockopt(sk, SOL_HCI, HCI_DATA_DIR, &opt, sizeof(opt)) < 0) {
		perror("Can't enable data direction info");
		return -1;
	}

	opt = 1;
	if (setsockopt(sk, SOL_HCI, HCI_TIME_STAMP, &opt, sizeof(opt)) < 0) {
		perror("Can't enable time stamp");
		return -1;
	}

	/* Setup filter */
	hci_filter_clear(&flt);
	hci_filter_all_ptypes(&flt);
	hci_filter_all_events(&flt);
	if (setsockopt(sk, SOL_HCI, HCI_FILTER, &flt, sizeof(flt)) < 0) {
		perror("Can't set filter");
		return -1;
	}

	/* Bind socket to the HCI device */
	memset(&addr, 0, sizeof(addr));
	addr.hci_family = AF_BLUETOOTH;
	addr.hci_dev = dev;
	if (bind(sk, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		printf("Can't attach to device hci%d. %s(%d)\n", 
					dev, strerror(errno), errno);
		return -1;
	}

	return sk;
}


#define BUFFER_SIZE 2048
int check_for_conn_req(char buf[BUFFER_SIZE], bdaddr_t *new_device)
{
	int i = 0;
	i = 0;
		
	for(i=0; i<BUFFER_SIZE; i++)
	{
		if(buf[i] == 0x04)
			if(buf[i+1] == 0x04)
				if(buf[i+2] == 0x0a)
					if(buf[i+9] == 0x00)
						if(buf[i+10] == 0x1f)
						{
							new_device->b[0] = (int)buf[i+3];
							new_device->b[1] = (int)buf[i+4];
							new_device->b[2] = (int)buf[i+5];
							new_device->b[3] = (int)buf[i+6];
							new_device->b[4] = (int)buf[i+7];
							new_device->b[5] = (int)buf[i+8];
							//printf("new device discovered=%s\n", tmp);
							//str2ba(tmp, new_device);
							return 1;
						}

	}
	return -1;
}

int device_add_to_list(char *addr, char *name, int type)
{
	//cur_dev_t *dev = NULL;
	device_t *new_dev = NULL;
	device_t *dev = NULL;
	/* remove found */
	int found;

	pthread_mutex_lock(&device_list_lock);
	printf("device list locked [%d] \n", __LINE__);

	/* search device in our list, if exist dont add it */ 
	for(dev = device_list; dev != NULL; dev = dev->next)
	{
		printd("check if same -> %s:%s\n", addr, dev->dev_id);
		if ( strcmp(addr, dev->dev_id) == 0)
			found = 1;
	}

		
	// TODO : update name field if it is set to [unknown]
	/* is it on the list */
	if (found == 1)
	{
		pthread_mutex_unlock(&device_list_lock);
		printf("device list unlocked [%d] \n", __LINE__);
		return -1;
	}

   	printd("adding device into list %s  %s\n", addr, name);

	new_dev = device_list_add(&device_list, addr, type);
	/* todo check if null */

	new_dev->state = DEV_NEARBY;
	new_dev->name = strdup(name);
	new_dev->dev_id = strdup(addr);
	new_dev->tid = -1;
	new_dev->latest_conn_time = time(NULL);
	new_dev->latest_conn_result = 1;
	new_dev->result = NULL;
	new_dev->delete_after_n_sec = -1;
	new_dev->try_after_n_sec = -1;

	/* if a new device is added donot scan again for 8 seconds */
    next_scan_nearby_delay = 8000000; // 8 seconds
	pthread_mutex_unlock(&device_list_lock);
	printf("device list unlocked [%d] \n", __LINE__);
	return 0;
}


int device_remove_from_list(char *addr)
{
	device_t *dev;
	char *tmp_addr = strdup(addr);
	/* remove found */
	int found;

	if (*addr == NULL){
		return -1;
	}

	printd("target device is %s\n", tmp_addr);
	pthread_mutex_lock(&device_list_lock);
	printd("device list locked [%d] \n", __LINE__);
		/* search device in our list, if exist dont add it */ 
		for(dev = device_list; dev != NULL; (dev) = (dev)->next)
		{
			if (strcmp(addr, (dev)->dev_id) == 0)
			{
				found = 1;
				device_list_remove(device_list_search(&device_list, dev->dev_id));
				
				printd("device removed from list -> %s\n", tmp_addr);
				break;
			}
		}
	pthread_mutex_unlock(&device_list_lock);
	printd("device list unlocked[%d]\n",__LINE__);
	free(tmp_addr);	
		// TODO : update name field if it is set to [unknown]
		/* is it on the list */
		if (found == 1)
			return 0;
		else
		{
			printf("device not on the list\n");
			return -1;
		}
	return 0;
}

int device_print_list(void)
{
	device_t *dev = NULL;
	/* remove found */

	pthread_mutex_lock(&device_list_lock);
//	printf("device list locked [%d] \n", __LINE__);
//	printf("*** Current Devices:");
		/* search device in our list, if exist dont add it */ 
		for(dev = device_list; dev != NULL; dev = dev->next)
		{
			printf(" [%s@%p]", dev->dev_id, dev);
		}
	printf("\n");
	pthread_mutex_unlock(&device_list_lock);
//	printf("device list unlocked[%d]\n",__LINE__);
//TRACE;
	return 0;
}



void *wait_for_new_device(void)
{
	char buf[BUFFER_SIZE];
	int sk;
	int numbytes, len=0;
	char *bufptr = (char *)&buf;
	struct timeval tv;
	struct pollfd fds;
	char new_device_str[18];
	bdaddr_t new_device;

	sk = open_socket(0,0);
	
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	printd("waiting for new device to connect...\n");
	fds.fd = sk;
	fds.events = POLLIN | POLLPRI;

	while(1)
	{
		if ( poll(&fds, 1, 100) > 0)
		{
			numbytes = recv(sk, bufptr, BUFFER_SIZE, 0);
			bufptr += numbytes;
			len += numbytes;

			if ( len > (BUFFER_SIZE - 1))
			{
	//			printf("Warning, Buffer area exceeded [%s:%d]\n", __func__, __LINE__);
					bufptr = (char *)&buf;
					len = 0;
				continue;			
	//			exit(0);
			}
			//printf("received some bt events [%d bytes]\n", numbytes);
		}
		else
		{
			if (len > 0)
			{
				if (check_for_conn_req(buf, &new_device) > 0)
				{
					bufptr = (char *)&buf;
					len = 0;
					ba2str(&new_device, (char *)&new_device_str);
					printf("****************%d::%s:: new device %s\n", __LINE__, __func__, new_device_str);
					device_add_to_list(new_device_str, "[unknown]", 0);
					continue;
				}
				else
					continue;
			}
			else
				continue;
		}				
	}

	/* never */
	return -1;
}

void *scan_nearby(void)
{
    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i,found;
    char addr[19] = { 0 };
    char name[248] = { 0 };
	int stop = 1;

	while(1){
	printd("next_scan_nearby_delay is %d\n", next_scan_nearby_delay);
	usleep(next_scan_nearby_delay);
	if (next_scan_nearby_delay > 100000)
		next_scan_nearby_delay = 100000; 
	printd("scanning nearby devices...\n");
	pthread_mutex_lock(&hci_inquiry_lock);

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));
    
    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) 
	{
	 perror("hci_inquiry");
	 usleep(500000);	
	}

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name), 
            name, 0) < 0)
        strcpy(name, "[unknown]");
        printf("%s  %s\n", addr, name);
		// demo hack
        if ((strcmp(addr, "00:1C:97:33:00:8C") == 0) || (strcmp(addr, "00:80:25:00:FE:2A") == 0))
		  device_add_to_list(addr, name, 0);		
    }

    free( ii );
    close( sock );
	pthread_mutex_unlock(&hci_inquiry_lock);
	}
	/* never */ 
	return;
}


void clear_local_notification(int secs)
{
	char cmd[64] = {0};
TRACE;
	sprintf(&cmd,"echo \"0\" > /var/btagentmsg");
TRACE;
	printf("Running command: %s in %d seconds \n", cmd, secs);
TRACE;
	sleep(secs);
TRACE;
	system(cmd);
}

void send_local_notification(char *msg, int delay)
{
	pthread_t delmsg_thread;
TRACE;
	char cmd[1024] = {0};
TRACE;
	sprintf(&cmd,"echo \"<div class=\\\"box\\\"><h2>%s</h2></div>\" > /var/btagentmsg", msg);
TRACE;
	printf("Running command: %s\n", cmd);
TRACE;
	system(cmd);
	pthread_create(&delmsg_thread, NULL, clear_local_notification, delay);
TRACE;
}


int *process_device(device_t *dev)
{
	client_t *client;
	kvlist_t *result=NULL;
	int retval = 0;
	char tmp_msg[512]={0};

	printd("new device appeared [%s, tid = %d] \n", dev->dev_id, dev->tid);
TRACE;
	if (attach_drivers(dev) < 0)
	{
		printf("Fatal Error, Suitable driver not found\n");
		exit(EXIT_FAILURE);
	}
TRACE;
	
	if (dev->status == DEV_ALIEN)
		goto Exception;

TRACE;
	sprintf(&tmp_msg, "Cihaz algılandı  [%s]", dev->name);
	send_local_notification(&tmp_msg, 5);
	if (dev->init(dev) < 0)
	{
			printf("%s device init failed.\n", dev->driver);
			retval = -1;
			goto Exception;
	}	
TRACE;

	if (dev->listen(dev) != DEV_MEAS_VALID)
	{
		printf("%s device set failed.\n", dev->driver);
		retval = -3;
		goto Exception;
	}
TRACE;
	sprintf(&tmp_msg, "SYS:<span>%s    </span>  DIA:<span>%s    </span>  HB:%s\n",
					kvlist_get((dev->result), "sys"),
					kvlist_get((dev->result), "dia"),
					kvlist_get((dev->result), "hb") );
	send_local_notification(&tmp_msg, 5);
	sleep(5);
	
	sprintf(&tmp_msg, "Ölçüm kaydediliyor...");
	send_local_notification(&tmp_msg, 5);

	dev->latest_conn_time = time(NULL);
	if (dev->halt(dev) < 0)
	{
		printf("%s device halt failed.\n", dev->driver);
		retval = -4;
		goto Exception;
	}
TRACE;
				
	kvlist_add(&(dev->result), "email", email);
	kvlist_add(&(dev->result), "password", password);

//	printf("Result:\n");
//	kvlist_print(dev->result);
TRACE;
				
	client = get_client(client_name);
	if (client == NULL)
	{
		printf("Fatal Error, Suitable client not found\n");
		exit(EXIT_FAILURE);
	}

	if (client->init(client) < 0)
	{
		printf("%s client init failed.\n", client_name);
		retval = -2;
		goto Exception;
	}	
	
	if (client->send("admin@admin.com", "123123", &(dev->result)) < 0 )
	{
		printf("%s client set failed.\n", client_name);
		retval = -3;
		goto Exception;
	}

	sprintf(&tmp_msg, "Ölçüm sunucuya kaydedildi");
	send_local_notification(&tmp_msg, 5);
	if (client->halt(client) < 0)
	{
		printf("%s client halt failed.\n", client_name);
		retval = -4;
		goto Exception;
	}	

	printd("%s device assigned as DELETE_AFTER_N_SEC\n", dev->dev_id);
	dev->latest_conn_time = time(NULL);
	dev->latest_conn_result = retval;
	dev->tid = -1;
	dev->state = DEV_DELETE_AFTER_N_SEC;

	/* if this timeout is not set by driver */
	if (dev->delete_after_n_sec == -1)
		dev->delete_after_n_sec = 30;

	return retval;				

Exception:
	dev->latest_conn_time = time(NULL);
	dev->latest_conn_result = retval;
	if (dev->state == DEV_ALIEN)
	{
		printd("%s device assigned as ALIEN\n", dev->dev_id);
		dev->state = DEV_DELETE_AFTER_N_SEC;

		/* if this timeout is not set by driver */
		if (dev->delete_after_n_sec == -1)
			dev->delete_after_n_sec = 90;

	}
	else if (dev->state == DEV_TRY_AFTER_N_SEC)
	{
		printd("%s device assigned as DELETE_AFTER_N_SEC\n", dev->dev_id);
		dev->state = DEV_DELETE_AFTER_N_SEC;

		/* if this timeout is not set by driver */
		if (dev->delete_after_n_sec == -1)
			dev->delete_after_n_sec = 15;
	}
	else
	{
		printd("%s device assigned as TRY_AFTER_N_SEC\n", dev->dev_id);
		dev->state = DEV_TRY_AFTER_N_SEC;

		/* if this timeout is not set by driver */
		if (dev->try_after_n_sec == -1)
			dev->try_after_n_sec = 30;
	}

	dev->tid = -1;
	return retval;

}



// cur_devs diye bir liste olmali
// buradaki cihazlar main thread de surekli denenmeli
// yeni bir geldiginde hemen isleme konmali
// her cihaz icin tekrar deneme suresi tutulmali o sure sonunda tekrar denenmeli
// boylece yeni gelen cihaz hemen isleme konmali
#define NUM_THREADS	10
//current devices, new devices
// if a new device received, a global flag should be set
// we should process based on mac id, after process it should be added to current devices
// if a device in current devices disappear it should be removed from list
// at least 2 threads,1 main program, 2 global list, 1 flag
int main(int argc, char *argv[])
{
	pthread_t threads[NUM_THREADS];
	device_t *dev = NULL;
	time_t cur_time;
	int only_test = 0;
	int rc,c;
	int t;

	printf("device list unlocked [%d] \n", __LINE__);

    if(argc<2)
    {    
        print_usage();
		return 0;
    }

    opterr = 0;

    while ((c = getopt (argc, argv, "he:p:c:d:tv")) != -1)
    switch (c)
    {

		case 'h':
			print_usage();
			break;

		case 't':
			only_test = 1;
			break;

        case 'e':
	  		email = strdup(optarg);
	  		break;

        case 'p':
	  		password = strdup(optarg);
	  		break;

        case 'c':
	  		client_name = strdup(optarg);
	  		break;

        case 'v':
	  		verbose = 1;
	  		break;

		default:
			print_usage();
			break;

    }

	if (client_name == NULL)
	{
		print_usage();
		return 0;
	}

	if (email == NULL)
		email = strdup("admin@admin.com");

	if (password == NULL)
		password = strdup("123123");

	printd("Current parameters, email=%s password=%s client_name=%s\n", 
			 email, password, client_name);
	

	if (only_test)
		device_add_to_list("AA:AA:AA:AA:AA:AA", "hello", 0);
	else {
		rc = 0;
		rc = pthread_create(&threads[0], NULL, scan_nearby, NULL);
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}
#if 0

	rc = 0;
	rc = pthread_create(&threads[1], NULL, wait_for_new_device, NULL);
	if (rc){
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
#endif

	while(1)
	{
//		printd("main thread is running\n");
		sleep(1);
		t = 2;
	//	pthread_mutex_lock(&device_list_lock);
//	printf("device list locked[%d]\n", __LINE__);

//		device_print_list();
		
		if ( (only_test) && (device_list == NULL) )
		{
			printf("test completed\n");
			return 0;
        }

		for(dev = device_list; dev != NULL; dev = dev->next)
		{
	//		printf("nearby_bevice = %s\n", dev->dev_id);
			
			if (dev->tid != -1)
				continue;

			/* device currently is in process */
			if (dev->state == DEV_NEARBY)
			{
					rc = 0;
					dev->tid = t;
					rc = pthread_create(&threads[t], NULL, process_device, (void*)dev);
					if (rc != 0){
						printf("ERROR; return code from pthread_create() is %d\n", rc);
					}
					else
						t++;
			}
			
			/* device currently is in process */
			if (dev->state == DEV_DELETE_AFTER_N_SEC)
			{
				cur_time = time(NULL);
				if (cur_time - dev->latest_conn_time > dev->delete_after_n_sec)
				{
					device_remove_from_list(dev->dev_id);
					break;
				}
			}

			/* device currently is in process */
			if (dev->state == DEV_TRY_AFTER_N_SEC)
			{
				cur_time = time(NULL);
				if (cur_time - dev->latest_conn_time > dev->try_after_n_sec)
				{
					rc = 0;
					dev->tid = t;
					rc = pthread_create(&threads[t], NULL, process_device, (void*)dev);
					if (rc != 0){
						printf("ERROR; return code from pthread_create() is %d\n", rc);
					}
					else
						t++;
				}
				break;
			}


	//	pthread_mutex_unlock(&device_list_lock);
	//	printf("device list unlocked [%d]\n", __LINE__);


		}
	}

    return 0;
}

