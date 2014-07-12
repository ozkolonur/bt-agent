#ifndef __KVLIST_H__
#define __KVLIST_H__

typedef struct kvlist_s {
        char *key;
		char *value;
        struct kvlist_s *next;
} kvlist_t;

kvlist_t *kvlist_add(kvlist_t **p, char *key, char *value);
void kvlist_remove(kvlist_t **p);
kvlist_t **kvlist_search(kvlist_t **n, char *key);
void kvlist_print(kvlist_t *n);
char *kvlist_get(kvlist_t *kvlist, char *key);
#define kvlist_del(list, key)  kvlist_remove(kvlist_search(list, key));

#endif /* __KVLIST_H__ */