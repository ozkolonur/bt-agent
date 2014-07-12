#include <stdio.h>   /* for printf */
#include <stdlib.h>  /* for malloc */
#include <string.h>
#include "kvlist.h"
 
kvlist_t *kvlist_add(kvlist_t **p, char *key, char *value) {
    /* some compilers don't require a cast of return value for malloc */
    kvlist_t *n = (kvlist_t *)malloc(sizeof(kvlist_t));
    if (n == NULL)
        return NULL;

	memset(n, 0, sizeof(kvlist_t));
    n->next = *p;                                                                            
    *p = n;
    n->key = strdup(key);
    n->value = strdup(value);
    return n;
}
 
void kvlist_remove(kvlist_t **p) { /* remove head */
    if (*p != NULL) {
        kvlist_t *n = *p;
        *p = (*p)->next;
        free(n->key);
        free(n->value);
        free(n);
    }
}
 
kvlist_t **kvlist_search(kvlist_t **n, char *key) {
    while (*n != NULL) {
        if (strcmp((*n)->key, key) == 0) {
            return n;
        }
        n = &(*n)->next;
    }
    return NULL;
}
 
void kvlist_print(kvlist_t *n) {
    if (n == NULL) {
        printf("list is empty\n");
    }
    while (n != NULL) {
        printf("print %s %s\n", n->key, n->value);
        n = n->next;
    }
}

char *kvlist_get(kvlist_t *kvlist, char *key)
{
	kvlist_t **item = NULL;
	item = kvlist_search(&kvlist, key);
	return (*item)->value;
}

 
#if 0
int main(void) {
    kvlist_t *n = NULL;
	kvlist_t **x = NULL;

    list_add(&n, "key0", "value0"); /* list: 0 */
	printf("------------%p\n", n);
    list_add(&n, "key1", "value1"); /* list: 1 0 */
	printf("------------%p\n", n);
    list_add(&n, "key2","value2"); /* list: 2 1 0 */
	printf("------------%p\n", n);
    list_add(&n, "key3","value3"); /* list: 3 2 1 0 */
	printf("------------%p\n", n);
    list_add(&n, "key4", "value4"); /* list: 4 3 2 1 0 */
	printf("------------%p\n", n);
    list_print(n);
	printf("------------");
//    list_remove(&n);            /* remove first (4) */
//    list_remove(&n->next);      /* remove new second (2) */
    //list_remove(list_search(&n, "key2")); /* remove cell containing 1 (first) */
//    list_remove(&n->next);      /* remove second to last kvlist_t (0) */
//    list_remove(&n);            /* remove last (3) */
//    list_print(n);
	printf("------------%p\n", n);
	x = list_search(&n, "key3");
	list_remove(x);
	printf("%s\n", (*x)->key); 	
    list_print(n);
    return 0;
}
#endif
