#include "stlport_prefix.h"

#if defined(__unix) && defined(__GNUC__)

#ifdef __FreeBSD__
#  include <osreldate.h>
#endif

#if (defined(__FreeBSD__) && (__FreeBSD_version < 503001)) || defined(__sun) || defined (__hpux)


#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
















enum {
  ef_free, 
  ef_us,
  ef_on,
  ef_at,
  ef_cxa
};

struct exit_function
{
  

  long int flavor;
  union {
    void (*at)(void);
    struct {
      void (*fn)(int status, void *arg);
      void *arg;
    } on;
    struct {
      void (*fn)(void *arg, int status);
      void *arg;
      void *dso_handle;
    } cxa;
  } func;
};

struct exit_function_list
{
  struct exit_function_list *next;
  size_t idx;
  struct exit_function fns[32];
};

struct exit_function *__new_exitfn (void);




int __cxa_atexit(void (*func)(void *), void *arg, void *d)
{
  struct exit_function *new = __new_exitfn ();

  if ( new == NULL )
    return -1;

  new->flavor = ef_cxa;
  new->func.cxa.fn = (void (*) (void *, int)) func;
  new->func.cxa.arg = arg;
  new->func.cxa.dso_handle = d;
  return 0;
}



#ifdef __linux__
static pthread_mutex_t lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif

#if 0
static pthread_mutex_t lock =
  { PTHREAD_MUTEX_RECURSIVE , PTHREAD_PRIO_NONE, {NULL,NULL},
    NULL, { NULL },  0x1, 0, 0, 0, {NULL, NULL},
    { 0, 0, 0, 0 } };
#endif
#ifdef __sun
static pthread_mutex_t lock =
  {{0, 0, 0, PTHREAD_MUTEX_RECURSIVE, _MUTEX_MAGIC}, {{{0}}}, 0};
#endif
#ifdef __hpux
static pthread_mutex_t lock = PTHREAD_MUTEX_RECURSIVE_INITIALIZER_NP;
#  ifdef __ia64
void *__dso_handle = (void *) &__dso_handle;
#  endif
#endif


static struct exit_function_list initial;
struct exit_function_list *__exit_funcs = &initial;

struct exit_function *__new_exitfn(void)
{
  struct exit_function_list *l;
  size_t i = 0;

#ifndef __FreeBSD__
  pthread_mutex_lock( &lock );
#endif

  for (l = __exit_funcs; l != NULL; l = l->next) {
    for (i = 0; i < l->idx; ++i)
      if (l->fns[i].flavor == ef_free)
        break;
    if ( i < l->idx )
      break;

    if (l->idx < sizeof (l->fns) / sizeof (l->fns[0])) {
      i = l->idx++;
      break;
    }
  }

  if (l == NULL) {
    l = (struct exit_function_list *)malloc( sizeof(struct exit_function_list) );
    if (l != NULL) {
      l->next = __exit_funcs;
      __exit_funcs = l;

      l->idx = 1;
      i = 0;
    }
  }

  
  if ( l != NULL )
    l->fns[i].flavor = ef_us;

#ifndef __FreeBSD__
  pthread_mutex_unlock( &lock );
#endif

  return l == NULL ? NULL : &l->fns[i];
}









void __cxa_finalize(void *d)
{
  struct exit_function_list *funcs;

#ifndef __FreeBSD__
  pthread_mutex_lock( &lock );
#endif

  for (funcs = __exit_funcs; funcs; funcs = funcs->next) {
    struct exit_function *f;

    for (f = &funcs->fns[funcs->idx - 1]; f >= &funcs->fns[0]; --f) {
      if ( (d == NULL || d == f->func.cxa.dso_handle) && (f->flavor == ef_cxa) ) {
        f->flavor = ef_free;
        (*f->func.cxa.fn) (f->func.cxa.arg, 0);
      }
    }
  }

  

#ifdef UNREGISTER_ATFORK
  if (d != NULL)
    UNREGISTER_ATFORK (d);
#endif
#ifndef __FreeBSD__
  pthread_mutex_unlock( &lock );
#endif
}




#endif 
#endif 

