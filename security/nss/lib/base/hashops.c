



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: hashops.c,v $ $Revision: 1.7 $ $Date: 2012/04/25 14:49:26 $";
#endif 







#ifndef BASE_H
#include "base.h"
#endif 

static void * PR_CALLBACK
nss_arena_hash_alloc_table
(
  void *pool,
  PRSize size
)
{
  NSSArena *arena = (NSSArena *)NULL;

#ifdef NSSDEBUG
  if( (void *)NULL != arena ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
      return (void *)NULL;
    }
  }
#endif 

  return nss_ZAlloc(arena, size);
}

static void PR_CALLBACK
nss_arena_hash_free_table
(
  void *pool, 
  void *item
)
{
  (void)nss_ZFreeIf(item);
}

static PLHashEntry * PR_CALLBACK
nss_arena_hash_alloc_entry
(
  void *pool,
  const void *key
)
{
  NSSArena *arena = NULL;

#ifdef NSSDEBUG
  if( (void *)NULL != arena ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arena) ) {
      return (void *)NULL;
    }
  }
#endif 

  return nss_ZNEW(arena, PLHashEntry);
}

static void PR_CALLBACK
nss_arena_hash_free_entry
(
  void *pool,
  PLHashEntry *he,
  PRUintn flag
)
{
  if( HT_FREE_ENTRY == flag ) {
    (void)nss_ZFreeIf(he);
  }
}

NSS_IMPLEMENT_DATA PLHashAllocOps 
nssArenaHashAllocOps = {
  nss_arena_hash_alloc_table,
  nss_arena_hash_free_table,
  nss_arena_hash_alloc_entry,
  nss_arena_hash_free_entry
};
