



#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: tracker.c,v $ $Revision: 1.8 $ $Date: 2012/04/25 14:49:26 $";
#endif 









#ifndef BASE_H
#include "base.h"
#endif 

#ifdef DEBUG








static PLHashNumber PR_CALLBACK
identity_hash
(
  const void *key
)
{
  return (PLHashNumber)key;
}









static PRStatus
trackerOnceFunc
(
  void *arg
)
{
  nssPointerTracker *tracker = (nssPointerTracker *)arg;

  tracker->lock = PZ_NewLock(nssILockOther);
  if( (PZLock *)NULL == tracker->lock ) {
    return PR_FAILURE;
  }

  tracker->table = PL_NewHashTable(0, 
                                   identity_hash, 
                                   PL_CompareValues,
                                   PL_CompareValues,
                                   (PLHashAllocOps *)NULL, 
                                   (void *)NULL);
  if( (PLHashTable *)NULL == tracker->table ) {
    PZ_DestroyLock(tracker->lock);
    tracker->lock = (PZLock *)NULL;
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}






















NSS_IMPLEMENT PRStatus
nssPointerTracker_initialize
(
  nssPointerTracker *tracker
)
{
  PRStatus rv = PR_CallOnceWithArg(&tracker->once, trackerOnceFunc, tracker);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
  }

  return rv;
}

#ifdef DONT_DESTROY_EMPTY_TABLES









static PRIntn PR_CALLBACK
count_entries
(
  PLHashEntry *he,
  PRIntn index,
  void *arg
)
{
  return HT_ENUMERATE_NEXT;
}
#endif 








static const PRCallOnceType zero_once;


























NSS_IMPLEMENT PRStatus
nssPointerTracker_finalize
(
  nssPointerTracker *tracker
)
{
  PZLock *lock;

  if( (nssPointerTracker *)NULL == tracker ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return PR_FAILURE;
  }

  if( (PZLock *)NULL == tracker->lock ) {
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  lock = tracker->lock;
  PZ_Lock(lock);

  if( (PLHashTable *)NULL == tracker->table ) {
    PZ_Unlock(lock);
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

#ifdef DONT_DESTROY_EMPTY_TABLES
  



  count = PL_HashTableEnumerateEntries(tracker->table, 
                                       count_entries,
                                       (void *)NULL);

  if( 0 != count ) {
    PZ_Unlock(lock);
    nss_SetError(NSS_ERROR_TRACKER_NOT_EMPTY);
    return PR_FAILURE;
  }
#endif 

  PL_HashTableDestroy(tracker->table);
  
  tracker->once = zero_once;
  tracker->lock = (PZLock *)NULL;
  tracker->table = (PLHashTable *)NULL;

  PZ_Unlock(lock);
  PZ_DestroyLock(lock);

  return PR_SUCCESS;
}
























NSS_IMPLEMENT PRStatus
nssPointerTracker_add
(
  nssPointerTracker *tracker,
  const void *pointer
)
{
  void *check;
  PLHashEntry *entry;

  if( (nssPointerTracker *)NULL == tracker ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return PR_FAILURE;
  }

  if( (PZLock *)NULL == tracker->lock ) {
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  PZ_Lock(tracker->lock);

  if( (PLHashTable *)NULL == tracker->table ) {
    PZ_Unlock(tracker->lock);
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  check = PL_HashTableLookup(tracker->table, pointer);
  if( (void *)NULL != check ) {
    PZ_Unlock(tracker->lock);
    nss_SetError(NSS_ERROR_DUPLICATE_POINTER);
    return PR_FAILURE;
  }

  entry = PL_HashTableAdd(tracker->table, pointer, (void *)pointer);

  PZ_Unlock(tracker->lock);

  if( (PLHashEntry *)NULL == entry ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}
  























NSS_IMPLEMENT PRStatus
nssPointerTracker_remove
(
  nssPointerTracker *tracker,
  const void *pointer
)
{
  PRBool registered;

  if( (nssPointerTracker *)NULL == tracker ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return PR_FAILURE;
  }

  if( (PZLock *)NULL == tracker->lock ) {
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  PZ_Lock(tracker->lock);

  if( (PLHashTable *)NULL == tracker->table ) {
    PZ_Unlock(tracker->lock);
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  registered = PL_HashTableRemove(tracker->table, pointer);
  PZ_Unlock(tracker->lock);

  if( !registered ) {
    nss_SetError(NSS_ERROR_POINTER_NOT_REGISTERED);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}


























NSS_IMPLEMENT PRStatus
nssPointerTracker_verify
(
  nssPointerTracker *tracker,
  const void *pointer
)
{
  void *check;

  if( (nssPointerTracker *)NULL == tracker ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return PR_FAILURE;
  }

  if( (PZLock *)NULL == tracker->lock ) {
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  PZ_Lock(tracker->lock);

  if( (PLHashTable *)NULL == tracker->table ) {
    PZ_Unlock(tracker->lock);
    nss_SetError(NSS_ERROR_TRACKER_NOT_INITIALIZED);
    return PR_FAILURE;
  }

  check = PL_HashTableLookup(tracker->table, pointer);
  PZ_Unlock(tracker->lock);

  if( (void *)NULL == check ) {
    nss_SetError(NSS_ERROR_POINTER_NOT_REGISTERED);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}

#endif 
