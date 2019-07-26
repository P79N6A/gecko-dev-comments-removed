


















































































#ifndef _r_list_h
#define _r_list_h

typedef struct r_list_ r_list;

typedef struct r_list_iterator_ {
     r_list *list;
     struct r_list_el_ *ptr;
} r_list_iterator;

int r_list_create(r_list **listp);
int r_list_destroy(r_list **listp);
int r_list_copy(r_list **out,r_list *in);
int r_list_insert(r_list *list,void *value,
  int (*copy)(void **knew,void *old),
  int (*destroy)(void **ptr));
int r_list_append(r_list *list,void *value,
  int (*copy)(void **knew,void *old),
  int (*destroy)(void **ptr));
int r_list_init_iter(r_list *list,r_list_iterator *iter);
int r_list_iter(r_list_iterator *iter,void **val);

#endif
