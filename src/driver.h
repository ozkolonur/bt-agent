#ifndef __DEVICE__H__
#define __DEVICE__H__

#include "agent.h"
#include "kvlist.h"

#define DEV_ERROR				-1
#define DEV_HALTED				0
#define DEV_INITALIZED			1
#define DEV_SCANNING			2
#define DEV_CONNECTING			3
#define DEV_CONNECTED			4
#define DEV_CONN_TIMEOUT 		6
#define DEV_MEAS_VALID			7
#define DEV_MEAS_INVALID		8
#define DEV_NEARBY				9
#define DEV_INPROCESS			10
#define DEV_DELETE_AFTER_N_SEC	11
#define DEV_TRY_AFTER_N_SEC		12
#define DEV_ALIEN				13
 
#define DEV_TYPE_BLUETOOTH		1
#define DEV_TYPE_USB			2
#define DEV_TYPE_DUMMY			3

typedef struct device_s device_t;

typedef struct device_s {
	char *name;
	int type;
	char *dev_id;
	int state;
	int status;
	struct device_s *next;
	struct device_s *self;
	int tid;
	int retval;
	char *driver;
	int (*init)(device_t *device);
	int (*listen)(device_t *device);
	int (*halt)(device_t *device);
	kvlist_t *result;
	time_t latest_conn_time;
	int latest_conn_result;
	int after_conn_timeout;
	int after_meas_timeout;
	int try_after_n_sec;
	int delete_after_n_sec;
};


// merge with cur_dev_t
// add device specific timeout value like : dev->listen_timeout
// status should be both exclusive and mutual like DEV_NEARBY || DEV_INITALIZED (bitwise)

typedef struct driver_s driver_t;

typedef struct driver_s {
	char *name;
	int (*init)(device_t *driver);
	int (*listen)(device_t *driver);
	int (*halt)(device_t *driver);
};

// merge with cur_dev_t
// add DRIVER specific timeout value like : dev->listen_timeout
// status should be both exclusive and mutual like DEV_NEARBY || DEV_INITALIZED (bitwise)

int attach_drivers(device_t *dev);


device_t *device_list_add(device_t **p, char *dev_id, int type);
device_t **device_list_search(device_t **n, char *dev_id);
void device_list_remove(device_t **p);


#endif /* __DEVICE_H__ */

