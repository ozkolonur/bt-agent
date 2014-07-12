#ifndef __LOGGER_H__
#define __LOGGER_H__

#include <stdarg.h>
#include <stdlib.h>

#ifndef THREADED_ENVIRONMENT
#define THREADED_ENVIRONMENT 1
#endif

typedef struct logger_s logger_t;

typedef struct logger_s{
	FILE *fd;
	int log_level;
	char *logger_id;
	void (*init)(logger_t *logger);
	void (*log)(logger_t *logger, const char *fmt, ...);
	void (*destroy)(logger_t *logger);
	struct logger_s *next;
};

logger_t *logger_list_add(char *logger_id);

void logger_list_remove(logger_t *p);

logger_t *get_logger(char *logger_id);

void logger_init(logger_t *logger);

void logger_log(logger_t *logger, const char *fmt, ...);

void logger_destroy(logger_t *logger);

#endif /* __LOGGER_H_ */