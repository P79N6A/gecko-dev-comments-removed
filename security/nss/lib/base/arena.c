



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: arena.c,v $ $Revision: 1.13 $ $Date: 2010/03/15 08:29:31 $";
#endif 







#ifndef BASE_H
#include "base.h"
#endif 

#ifdef ARENA_THREADMARK
#include "prthread.h"
#endif 

#include "prlock.h"
#include "plarena.h"

#include <string.h>






























struct NSSArenaStr {
  PLArenaPool pool;
  PRLock *lock;
#ifdef ARENA_THREADMARK
  PRThread *marking_thread;
  nssArenaMark *first_mark;
  nssArenaMark *last_mark;
#endif 
#ifdef ARENA_DESTRUCTOR_LIST
  struct arena_destructor_node *first_destructor;
  struct arena_destructor_node *last_destructor;
#endif 
};







struct nssArenaMarkStr {
  PRUint32 magic;
  void *mark;
#ifdef ARENA_THREADMARK
  nssArenaMark *next;
#endif 
#ifdef ARENA_DESTRUCTOR_LIST
  struct arena_destructor_node *next_destructor;
  struct arena_destructor_node *prev_destructor;
#endif 
};

#define MARK_MAGIC 0x4d41524b /* "MARK" how original */




#ifdef DEBUG
extern const NSSError NSS_ERROR_INTERNAL_ERROR;

static nssPointerTracker arena_pointer_tracker;

static PRStatus
arena_add_pointer
(
  const NSSArena *arena
)
{
  PRStatus rv;

  rv = nssPointerTracker_initialize(&arena_pointer_tracker);
  if( PR_SUCCESS != rv ) {
    return rv;
  }

  rv = nssPointerTracker_add(&arena_pointer_tracker, arena);
  if( PR_SUCCESS != rv ) {
    NSSError e = NSS_GetError();
    if( NSS_ERROR_NO_MEMORY != e ) {
      nss_SetError(NSS_ERROR_INTERNAL_ERROR);
    }

    return rv;
  }

  return PR_SUCCESS;
}

static PRStatus
arena_remove_pointer
(
  const NSSArena *arena
)
{
  PRStatus rv;

  rv = nssPointerTracker_remove(&arena_pointer_tracker, arena);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INTERNAL_ERROR);
  }

  return rv;
}


















NSS_IMPLEMENT PRStatus
nssArena_verifyPointer
(
  const NSSArena *arena
)
{
  PRStatus rv;

  rv = nssPointerTracker_initialize(&arena_pointer_tracker);
  if( PR_SUCCESS != rv ) {
    








    nss_SetError(NSS_ERROR_INVALID_ARENA);
    return PR_FAILURE;
  }

  rv = nssPointerTracker_verify(&arena_pointer_tracker, arena);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INVALID_ARENA);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}
#endif 

#ifdef ARENA_DESTRUCTOR_LIST

struct arena_destructor_node {
  struct arena_destructor_node *next;
  struct arena_destructor_node *prev;
  void (*destructor)(void *argument);
  void *arg;
};



























NSS_IMPLEMENT PRStatus
nssArena_registerDestructor
(
  NSSArena *arena,
  void (*destructor)(void *argument),
  void *arg
)
{
  struct arena_destructor_node *it;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
    return PR_FAILURE;
  }
#endif 
  
  it = nss_ZNEW(arena, struct arena_destructor_node);
  if( (struct arena_destructor_node *)NULL == it ) {
    return PR_FAILURE;
  }

  it->prev = arena->last_destructor;
  arena->last_destructor->next = it;
  arena->last_destructor = it;
  it->destructor = destructor;
  it->arg = arg;

  if( (nssArenaMark *)NULL != arena->last_mark ) {
    arena->last_mark->prev_destructor = it->prev;
    arena->last_mark->next_destructor = it->next;
  }

  return PR_SUCCESS;
}

NSS_IMPLEMENT PRStatus
nssArena_deregisterDestructor
(
  NSSArena *arena,
  void (*destructor)(void *argument),
  void *arg
)
{
  struct arena_destructor_node *it;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
    return PR_FAILURE;
  }
#endif 

  for( it = arena->first_destructor; it; it = it->next ) {
    if( (it->destructor == destructor) && (it->arg == arg) ) {
      break;
    }
  }

  if( (struct arena_destructor_node *)NULL == it ) {
    nss_SetError(NSS_ERROR_NOT_FOUND);
    return PR_FAILURE;
  }

  if( it == arena->first_destructor ) {
    arena->first_destructor = it->next;
  }

  if( it == arena->last_destructor ) {
    arena->last_destructor = it->prev;
  }

  if( (struct arena_destructor_node *)NULL != it->prev ) {
    it->prev->next = it->next;
  }

  if( (struct arena_destructor_node *)NULL != it->next ) {
    it->next->prev = it->prev;
  }

  {
    nssArenaMark *m;
    for( m = arena->first_mark; m; m = m->next ) {
      if( m->next_destructor == it ) {
        m->next_destructor = it->next;
      }
      if( m->prev_destructor == it ) {
        m->prev_destructor = it->prev;
      }
    }
  }

  nss_ZFreeIf(it);
  return PR_SUCCESS;
}

static void
nss_arena_call_destructor_chain
(
  struct arena_destructor_node *it
)
{
  for( ; it ; it = it->next ) {
    (*(it->destructor))(it->arg);
  }
}

#endif 















NSS_IMPLEMENT NSSArena *
NSSArena_Create
(
  void
)
{
  nss_ClearErrorStack();
  return nssArena_Create();
}
















NSS_IMPLEMENT NSSArena *
nssArena_Create
(
  void
)
{
  NSSArena *rv = (NSSArena *)NULL;

  rv = nss_ZNEW((NSSArena *)NULL, NSSArena);
  if( (NSSArena *)NULL == rv ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (NSSArena *)NULL;
  }

  rv->lock = PR_NewLock();
  if( (PRLock *)NULL == rv->lock ) {
    (void)nss_ZFreeIf(rv);
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (NSSArena *)NULL;
  }

  
















  PL_InitArenaPool(&rv->pool, "NSS", 2048, sizeof(double));

#ifdef DEBUG
  {
    PRStatus st;
    st = arena_add_pointer(rv);
    if( PR_SUCCESS != st ) {
      PL_FinishArenaPool(&rv->pool);
      PR_DestroyLock(rv->lock);
      (void)nss_ZFreeIf(rv);
      return (NSSArena *)NULL;
    }
  }
#endif 

  return rv;
}

















NSS_IMPLEMENT PRStatus
NSSArena_Destroy
(
  NSSArena *arena
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
    return PR_FAILURE;
  }
#endif 

  return nssArena_Destroy(arena);
}

















NSS_IMPLEMENT PRStatus
nssArena_Destroy
(
  NSSArena *arena
)
{
  PRLock *lock;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
    return PR_FAILURE;
  }
#endif 

  if( (PRLock *)NULL == arena->lock ) {
    
    nss_SetError(NSS_ERROR_INVALID_ARENA);
    return PR_FAILURE;
  }
  PR_Lock(arena->lock);
  
#ifdef DEBUG
  if( PR_SUCCESS != arena_remove_pointer(arena) ) {
    PR_Unlock(arena->lock);
    return PR_FAILURE;
  }
#endif 

#ifdef ARENA_DESTRUCTOR_LIST
  
  nss_arena_call_destructor_chain(arena->first_destructor);
#endif 

  PL_FinishArenaPool(&arena->pool);
  lock = arena->lock;
  arena->lock = (PRLock *)NULL;
  PR_Unlock(lock);
  PR_DestroyLock(lock);
  (void)nss_ZFreeIf(arena);
  return PR_SUCCESS;
}

static void *nss_zalloc_arena_locked(NSSArena *arena, PRUint32 size);






















NSS_IMPLEMENT nssArenaMark *
nssArena_Mark
(
  NSSArena *arena
)
{
  nssArenaMark *rv;
  void *p;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
    return (nssArenaMark *)NULL;
  }
#endif 

  if( (PRLock *)NULL == arena->lock ) {
    
    nss_SetError(NSS_ERROR_INVALID_ARENA);
    return (nssArenaMark *)NULL;
  }
  PR_Lock(arena->lock);

#ifdef ARENA_THREADMARK
  if( (PRThread *)NULL == arena->marking_thread ) {
    
    arena->marking_thread = PR_GetCurrentThread();
    
  } else {
    
    if( PR_GetCurrentThread() != arena->marking_thread ) {
      PR_Unlock(arena->lock);
      nss_SetError(NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD);
      return (nssArenaMark *)NULL;
    }
  }
#endif 

  p = PL_ARENA_MARK(&arena->pool);
  

  
  rv = (nssArenaMark *)nss_zalloc_arena_locked(arena, sizeof(nssArenaMark));
  if( (nssArenaMark *)NULL == rv ) {
    PR_Unlock(arena->lock);
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (nssArenaMark *)NULL;
  }

#ifdef ARENA_THREADMARK
  if ( (nssArenaMark *)NULL == arena->first_mark) {
    arena->first_mark = rv;
    arena->last_mark = rv;
  } else {
    arena->last_mark->next = rv;
    arena->last_mark = rv;
  }
#endif 

  rv->mark = p;
  rv->magic = MARK_MAGIC;

#ifdef ARENA_DESTRUCTOR_LIST
  rv->prev_destructor = arena->last_destructor;
#endif 

  PR_Unlock(arena->lock);

  return rv;
}








static PRStatus
nss_arena_unmark_release
(
  NSSArena *arena,
  nssArenaMark *arenaMark,
  PRBool release
)
{
  void *inner_mark;

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
    return PR_FAILURE;
  }
#endif 

  if( MARK_MAGIC != arenaMark->magic ) {
    nss_SetError(NSS_ERROR_INVALID_ARENA_MARK);
    return PR_FAILURE;
  }

  if( (PRLock *)NULL == arena->lock ) {
    
    nss_SetError(NSS_ERROR_INVALID_ARENA);
    return PR_FAILURE;
  }
  PR_Lock(arena->lock);

#ifdef ARENA_THREADMARK
  if( (PRThread *)NULL != arena->marking_thread ) {
    if( PR_GetCurrentThread() != arena->marking_thread ) {
      PR_Unlock(arena->lock);
      nss_SetError(NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD);
      return PR_FAILURE;
    }
  }
#endif 

  if( MARK_MAGIC != arenaMark->magic ) {
    
    PR_Unlock(arena->lock);
    nss_SetError(NSS_ERROR_INVALID_ARENA_MARK);
    return PR_FAILURE;
  }

  arenaMark->magic = 0;
  inner_mark = arenaMark->mark;

#ifdef ARENA_THREADMARK
  {
    nssArenaMark **pMark = &arena->first_mark;
    nssArenaMark *rest;
    nssArenaMark *last = (nssArenaMark *)NULL;

    
    while( *pMark != arenaMark ) {
      last = *pMark;
      pMark = &(*pMark)->next;
    }

    
    rest = (*pMark)->next;
    *pMark = (nssArenaMark *)NULL;

    arena->last_mark = last;

    
    for( ; (nssArenaMark *)NULL != rest; rest = rest->next ) {
      rest->magic = 0;
    }

    
    if( (nssArenaMark *)NULL == arena->first_mark ) {
      arena->marking_thread = (PRThread *)NULL;
    }
  }
#endif 

  if( release ) {
#ifdef ARENA_DESTRUCTOR_LIST
    if( (struct arena_destructor_node *)NULL != arenaMark->prev_destructor ) {
      arenaMark->prev_destructor->next = (struct arena_destructor_node *)NULL;
    }
    arena->last_destructor = arenaMark->prev_destructor;

    
    nss_arena_call_destructor_chain(arenaMark->next_destructor);
#endif 

    PR_ARENA_RELEASE(&arena->pool, inner_mark);
    
  }

  PR_Unlock(arena->lock);
  return PR_SUCCESS;
}




















NSS_IMPLEMENT PRStatus
nssArena_Release
(
  NSSArena *arena,
  nssArenaMark *arenaMark
)
{
  return nss_arena_unmark_release(arena, arenaMark, PR_TRUE);
}























NSS_IMPLEMENT PRStatus
nssArena_Unmark
(
  NSSArena *arena,
  nssArenaMark *arenaMark
)
{
  return nss_arena_unmark_release(arena, arenaMark, PR_FALSE);
}









struct pointer_header {
  NSSArena *arena;
  PRUint32 size;
};

static void *
nss_zalloc_arena_locked
(
  NSSArena *arena,
  PRUint32 size
)
{
  void *p;
  void *rv;
  struct pointer_header *h;
  PRUint32 my_size = size + sizeof(struct pointer_header);
  PR_ARENA_ALLOCATE(p, &arena->pool, my_size);
  if( (void *)NULL == p ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (void *)NULL;
  }
  




  h = (struct pointer_header *)p;
  h->arena = arena;
  h->size = size;
  rv = (void *)((char *)h + sizeof(struct pointer_header));
  (void)nsslibc_memset(rv, 0, size);
  return rv;
}
























NSS_IMPLEMENT void *
nss_ZAlloc
(
  NSSArena *arenaOpt,
  PRUint32 size
)
{
  struct pointer_header *h;
  PRUint32 my_size = size + sizeof(struct pointer_header);

  if( my_size < sizeof(struct pointer_header) ) {
    
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (void *)NULL;
  }

  if( (NSSArena *)NULL == arenaOpt ) {
    
    h = (struct pointer_header *)PR_Calloc(1, my_size);
    if( (struct pointer_header *)NULL == h ) {
      nss_SetError(NSS_ERROR_NO_MEMORY);
      return (void *)NULL;
    }

    h->arena = (NSSArena *)NULL;
    h->size = size;
    

    return (void *)((char *)h + sizeof(struct pointer_header));
  } else {
    void *rv;
    
#ifdef NSSDEBUG
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (void *)NULL;
    }
#endif 

    if( (PRLock *)NULL == arenaOpt->lock ) {
      
      nss_SetError(NSS_ERROR_INVALID_ARENA);
      return (void *)NULL;
    }
    PR_Lock(arenaOpt->lock);

#ifdef ARENA_THREADMARK
    if( (PRThread *)NULL != arenaOpt->marking_thread ) {
      if( PR_GetCurrentThread() != arenaOpt->marking_thread ) {
        nss_SetError(NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD);
        PR_Unlock(arenaOpt->lock);
        return (void *)NULL;
      }
    }
#endif 

    rv = nss_zalloc_arena_locked(arenaOpt, size);

    PR_Unlock(arenaOpt->lock);
    return rv;
  }
  
}



















NSS_IMPLEMENT PRStatus
nss_ZFreeIf
(
  void *pointer
)
{
  struct pointer_header *h;

  if( (void *)NULL == pointer ) {
    return PR_SUCCESS;
  }

  h = (struct pointer_header *)((char *)pointer
    - sizeof(struct pointer_header));

  

  if( (NSSArena *)NULL == h->arena ) {
    
    (void)nsslibc_memset(pointer, 0, h->size);
    PR_Free(h);
    return PR_SUCCESS;
  } else {
    
#ifdef NSSDEBUG
    if( PR_SUCCESS != nssArena_verifyPointer(h->arena) ) {
      return PR_FAILURE;
    }
#endif 

    if( (PRLock *)NULL == h->arena->lock ) {
      
      nss_SetError(NSS_ERROR_INVALID_POINTER);
      return PR_FAILURE;
    }
    PR_Lock(h->arena->lock);

    (void)nsslibc_memset(pointer, 0, h->size);

    

    PR_Unlock(h->arena->lock);
    return PR_SUCCESS;
  }
  
}





















NSS_EXTERN void *
nss_ZRealloc
(
  void *pointer,
  PRUint32 newSize
)
{
  NSSArena *arena;
  struct pointer_header *h, *new_h;
  PRUint32 my_newSize = newSize + sizeof(struct pointer_header);
  void *rv;

  if( my_newSize < sizeof(struct pointer_header) ) {
    
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (void *)NULL;
  }

  if( (void *)NULL == pointer ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (void *)NULL;
  }

  h = (struct pointer_header *)((char *)pointer
    - sizeof(struct pointer_header));

  

  if( newSize == h->size ) {
    
    return pointer;
  }

  arena = h->arena;
  if (!arena) {
    
    new_h = (struct pointer_header *)PR_Calloc(1, my_newSize);
    if( (struct pointer_header *)NULL == new_h ) {
      nss_SetError(NSS_ERROR_NO_MEMORY);
      return (void *)NULL;
    }

    new_h->arena = (NSSArena *)NULL;
    new_h->size = newSize;
    rv = (void *)((char *)new_h + sizeof(struct pointer_header));

    if( newSize > h->size ) {
      (void)nsslibc_memcpy(rv, pointer, h->size);
      (void)nsslibc_memset(&((char *)rv)[ h->size ], 
                           0, (newSize - h->size));
    } else {
      (void)nsslibc_memcpy(rv, pointer, newSize);
    }

    (void)nsslibc_memset(pointer, 0, h->size);
    h->size = 0;
    PR_Free(h);

    return rv;
  } else {
    void *p;
    
#ifdef NSSDEBUG
    if (PR_SUCCESS != nssArena_verifyPointer(arena)) {
      return (void *)NULL;
    }
#endif 

    if (!arena->lock) {
      
      nss_SetError(NSS_ERROR_INVALID_POINTER);
      return (void *)NULL;
    }
    PR_Lock(arena->lock);

#ifdef ARENA_THREADMARK
    if (arena->marking_thread) {
      if (PR_GetCurrentThread() != arena->marking_thread) {
        PR_Unlock(arena->lock);
        nss_SetError(NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD);
        return (void *)NULL;
      }
    }
#endif 

    if( newSize < h->size ) {
      











      char *extra = &((char *)pointer)[ newSize ];
      (void)nsslibc_memset(extra, 0, (h->size - newSize));
      PR_Unlock(arena->lock);
      return pointer;
    }

    PR_ARENA_ALLOCATE(p, &arena->pool, my_newSize);
    if( (void *)NULL == p ) {
      PR_Unlock(arena->lock);
      nss_SetError(NSS_ERROR_NO_MEMORY);
      return (void *)NULL;
    }

    new_h = (struct pointer_header *)p;
    new_h->arena = arena;
    new_h->size = newSize;
    rv = (void *)((char *)new_h + sizeof(struct pointer_header));
    if (rv != pointer) {
	(void)nsslibc_memcpy(rv, pointer, h->size);
	(void)nsslibc_memset(pointer, 0, h->size);
    }
    (void)nsslibc_memset(&((char *)rv)[ h->size ], 0, (newSize - h->size));
    h->arena = (NSSArena *)NULL;
    h->size = 0;
    PR_Unlock(arena->lock);
    return rv;
  }
  
}

PRStatus 
nssArena_Shutdown(void)
{
  PRStatus rv = PR_SUCCESS;
#ifdef DEBUG
  rv = nssPointerTracker_finalize(&arena_pointer_tracker);
#endif
  return rv;
}
