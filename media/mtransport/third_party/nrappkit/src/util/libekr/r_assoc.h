






















































































#ifndef _r_assoc_h
#define _r_assoc_h

typedef struct r_assoc_ r_assoc;

int r_assoc_create(r_assoc **assocp,
                             int (*hash_func)(char *,int,int),
                             int bits);
int r_assoc_insert(r_assoc *assoc,char *key,int len,
  void *value,int (*copy)(void **knew,void *old),
  int (*destroy)(void *ptr),int how);
#define R_ASSOC_REPLACE		  0x1
#define R_ASSOC_NEW		  0x2

int r_assoc_fetch(r_assoc *assoc,char *key, int len, void **value);
int r_assoc_delete(r_assoc *assoc,char *key, int len);

int r_assoc_copy(r_assoc **knew,r_assoc *old);
int r_assoc_destroy(r_assoc **assocp);
int r_assoc_simple_hash_compute(char *key, int len,int bits);
int r_assoc_crc32_hash_compute(char *key, int len,int bits);


typedef struct r_assoc_iterator_ {
     r_assoc *assoc;
     int prev_chain;
     struct r_assoc_el_ *prev;
     int next_chain;
     struct r_assoc_el_ *next;
} r_assoc_iterator;

int r_assoc_init_iter(r_assoc *assoc,r_assoc_iterator *);
int r_assoc_iter(r_assoc_iterator *iter,void **key,int *keyl, void **val);
int r_assoc_iter_delete(r_assoc_iterator *);

int r_assoc_num_elements(r_assoc *assoc,int *sizep);

#endif

