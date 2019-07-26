






































#ifndef _async_wait_int_h
#define _async_wait_int_h


typedef struct callback_ {
     NR_async_cb cb;
     void *arg;
     int how;
     int resource;
     void *backptr;

     char *func;
     int free_func;
     int line;
     TAILQ_ENTRY(callback_) entry;
} callback;

int nr_async_create_cb(NR_async_cb cb,void *arg,int how,
  int resource,char *func,int line,callback **cbp);
int nr_async_destroy_cb(callback **cbp);

#endif

