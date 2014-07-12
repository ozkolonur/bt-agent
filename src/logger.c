#include <stdio.h>
#include <stdarg.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "logger.h"

#ifdef THREADED_ENVIRONMENT
pthread_mutex_t logger_list_lock;
#endif
logger_t *logger_list = NULL;
 
logger_t *new_logger(char *logger_id)
{
	logger_t *logger = logger_list_add(logger_id);
    logger->init = logger_init;
    logger->log = logger_log;
    logger->destroy = logger_destroy;
    return logger;
}

logger_t *logger_list_add(char *logger_id) {
    logger_t *n = (logger_t *)malloc(sizeof(logger_t));

    if (n == NULL)
        return NULL;

#ifdef THREADED_ENVIRONMENT
	pthread_mutex_lock(&logger_list_lock);
#endif

    n->next = *(&logger_list);                                                                            
    logger_list = n;

#ifdef THREADED_ENVIRONMENT
	pthread_mutex_unlock(&logger_list_lock);
#endif

    n->logger_id = strdup(logger_id);
    return n;
}

void logger_list_remove(logger_t *p) { /* remove head */

	logger_t **n = &logger_list;
    while (*n != NULL) {
        if (strcmp((*n)->logger_id, p->logger_id) == 0) {

#ifdef THREADED_ENVIRONMENT
			pthread_mutex_lock(&logger_list_lock);
#endif
            (*n) = (*n)->next;

#ifdef THREADED_ENVIRONMENT
			pthread_mutex_unlock(&logger_list_lock);
#endif
			free(p->logger_id);
            free(p);
            return;
        }
        n = &(*n)->next;
    }
    return;
}

logger_t *get_logger(char *logger_id) {
	logger_t **n = &logger_list;
    while (*n != NULL) {
        if (strcmp((*n)->logger_id, logger_id) == 0) {
            return *n;
        }
        n = &(*n)->next;
    }
    return NULL;
}

void logger_init(logger_t *logger)
{
	char path[128] = {0};
	FILE *fd;

	sprintf((char *)&path, "/var/log/bt-agent/%s\n", logger->logger_id);
	
	fd = fopen(path, "w+");
	if (fd == NULL) {
	  perror("fopen");
	}
	else
	  logger->fd = fd;

    return;
}

void logger_log(logger_t *logger, const char *fmt, ...)
{
	va_list args;
  	va_start (args, fmt);
	vfprintf(logger->fd, fmt, args);
	va_end (args);
	return;
}

void logger_destroy(logger_t *logger)
{
    fclose(logger->fd);
  	logger_list_remove(logger);
	return;
}
