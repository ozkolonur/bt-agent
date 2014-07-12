#ifndef __AGENT_H__
#define __AGENT_H__

#define TRUE 	1
#define FALSE 	0
#define SUCCESS 0
#define ERROR 	-1

#include <pthread.h>
extern pthread_mutex_t hci_inquiry_lock;

#define TRACE printf("::%d::%s::%d::\n", __LINE__, __func__, (int)clock())

int verbose;

#ifdef VERBOSE_ENABLED
#define printd(args...) if (verbose)	printf(args)
#else
#define printd(args...)
#endif /* VERBOSE_ENABLED */

#endif /* __AGENT_H__ */
