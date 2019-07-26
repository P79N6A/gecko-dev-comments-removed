













#ifndef BASE_H
#include "base.h"
#endif 

#include "prbit.h"














struct nssHashStr {
  NSSArena *arena;
  PRBool i_alloced_arena;
  PRLock *mutex;

  




  PLHashTable *plHashTable;
  PRUint32 count;
};

static PLHashNumber
nss_identity_hash
(
  const void *key
)
{
  PRUint32 i = (PRUint32)key;
  PR_ASSERT(sizeof(PLHashNumber) == sizeof(PRUint32));
  return (PLHashNumber)i;
}

static PLHashNumber
nss_item_hash
(
  const void *key
)
{
  unsigned int i;
  PLHashNumber h;
  NSSItem *it = (NSSItem *)key;
  h = 0;
  for (i=0; i<it->size; i++)
    h = PR_ROTATE_LEFT32(h, 4) ^ ((unsigned char *)it->data)[i];
  return h;
}

static int
nss_compare_items(const void *v1, const void *v2)
{
  PRStatus ignore;
  return (int)nssItem_Equal((NSSItem *)v1, (NSSItem *)v2, &ignore);
}





NSS_IMPLEMENT nssHash *
nssHash_Create
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets,
  PLHashFunction keyHash,
  PLHashComparator keyCompare,
  PLHashComparator valueCompare
)
{
  nssHash *rv;
  NSSArena *arena;
  PRBool i_alloced;

#ifdef NSSDEBUG
  if( arenaOpt && PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
    nss_SetError(NSS_ERROR_INVALID_POINTER);
    return (nssHash *)NULL;
  }
#endif 

  if (arenaOpt) {
    arena = arenaOpt;
    i_alloced = PR_FALSE;
  } else {
    arena = nssArena_Create();
    i_alloced = PR_TRUE;
  }

  rv = nss_ZNEW(arena, nssHash);
  if( (nssHash *)NULL == rv ) {
    goto loser;
  }

  rv->mutex = PZ_NewLock(nssILockOther);
  if( (PZLock *)NULL == rv->mutex ) {
    goto loser;
  }

  rv->plHashTable = PL_NewHashTable(numBuckets, 
                                    keyHash, keyCompare, valueCompare,
                                    &nssArenaHashAllocOps, arena);
  if( (PLHashTable *)NULL == rv->plHashTable ) {
    (void)PZ_DestroyLock(rv->mutex);
    goto loser;
  }

  rv->count = 0;
  rv->arena = arena;
  rv->i_alloced_arena = i_alloced;

  return rv;
loser:
  (void)nss_ZFreeIf(rv);
  return (nssHash *)NULL;
}





NSS_IMPLEMENT nssHash *
nssHash_CreatePointer
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
)
{
  return nssHash_Create(arenaOpt, numBuckets, 
                        nss_identity_hash, PL_CompareValues, PL_CompareValues);
}





NSS_IMPLEMENT nssHash *
nssHash_CreateString
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
)
{
  return nssHash_Create(arenaOpt, numBuckets, 
                        PL_HashString, PL_CompareStrings, PL_CompareStrings);
}





NSS_IMPLEMENT nssHash *
nssHash_CreateItem
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
)
{
  return nssHash_Create(arenaOpt, numBuckets, 
                        nss_item_hash, nss_compare_items, PL_CompareValues);
}





NSS_IMPLEMENT void
nssHash_Destroy
(
  nssHash *hash
)
{
  (void)PZ_DestroyLock(hash->mutex);
  PL_HashTableDestroy(hash->plHashTable);
  if (hash->i_alloced_arena) {
    nssArena_Destroy(hash->arena);
  } else {
    nss_ZFreeIf(hash);
  }
}





NSS_IMPLEMENT PRStatus
nssHash_Add
(
  nssHash *hash,
  const void *key,
  const void *value
)
{
  PRStatus error = PR_FAILURE;
  PLHashEntry *he;

  PZ_Lock(hash->mutex);
  
  he = PL_HashTableAdd(hash->plHashTable, key, (void *)value);
  if( (PLHashEntry *)NULL == he ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
  } else if (he->value != value) {
    nss_SetError(NSS_ERROR_HASH_COLLISION);
  } else {
    hash->count++;
    error = PR_SUCCESS;
  }

  (void)PZ_Unlock(hash->mutex);

  return error;
}





NSS_IMPLEMENT void
nssHash_Remove
(
  nssHash *hash,
  const void *it
)
{
  PRBool found;

  PZ_Lock(hash->mutex);

  found = PL_HashTableRemove(hash->plHashTable, it);
  if( found ) {
    hash->count--;
  }

  (void)PZ_Unlock(hash->mutex);
  return;
}





NSS_IMPLEMENT PRUint32
nssHash_Count
(
  nssHash *hash
)
{
  PRUint32 count;

  PZ_Lock(hash->mutex);

  count = hash->count;

  (void)PZ_Unlock(hash->mutex);

  return count;
}





NSS_IMPLEMENT PRBool
nssHash_Exists
(
  nssHash *hash,
  const void *it
)
{
  void *value;

  PZ_Lock(hash->mutex);

  value = PL_HashTableLookup(hash->plHashTable, it);

  (void)PZ_Unlock(hash->mutex);

  if( (void *)NULL == value ) {
    return PR_FALSE;
  } else {
    return PR_TRUE;
  }
}





NSS_IMPLEMENT void *
nssHash_Lookup
(
  nssHash *hash,
  const void *it
)
{
  void *rv;

  PZ_Lock(hash->mutex);

  rv = PL_HashTableLookup(hash->plHashTable, it);

  (void)PZ_Unlock(hash->mutex);

  return rv;
}

struct arg_str {
  nssHashIterator fcn;
  void *closure;
};

static PRIntn
nss_hash_enumerator
(
  PLHashEntry *he,
  PRIntn index,
  void *arg
)
{
  struct arg_str *as = (struct arg_str *)arg;
  as->fcn(he->key, he->value, as->closure);
  return HT_ENUMERATE_NEXT;
}






NSS_IMPLEMENT void
nssHash_Iterate
(
  nssHash *hash,
  nssHashIterator fcn,
  void *closure
)
{
  struct arg_str as;
  as.fcn = fcn;
  as.closure = closure;

  PZ_Lock(hash->mutex);

  PL_HashTableEnumerateEntries(hash->plHashTable, nss_hash_enumerator, &as);

  (void)PZ_Unlock(hash->mutex);

  return;
}
