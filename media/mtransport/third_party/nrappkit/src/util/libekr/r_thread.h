






































#ifndef _r_thread_h
#define _r_thread_h

typedef void *r_thread;
typedef void *r_rwlock;
typedef void * r_cond;

int r_thread_fork (void (*func)(void *),void *arg,
  r_thread *tid);
int r_thread_destroy (r_thread tid);
int r_thread_yield (void);
int r_thread_exit (void);
int r_thread_wait_last (void);
int r_thread_self (void);

int r_rwlock_create (r_rwlock **lockp);
int r_rwlock_destroy (r_rwlock **lock);
int r_rwlock_lock (r_rwlock *lock,int action);

int r_cond_init (r_cond *cond);
int r_cond_wait (r_cond cond);
int r_cond_signal (r_cond cond);

#define R_RWLOCK_UNLOCK 0
#define R_RWLOCK_RLOCK 1
#define R_RWLOCK_WLOCK 2

#endif

