



































#ifdef DEBUG
static const char CVS_ID[] = "@(#) $RCSfile: oid.c,v $ $Revision: 1.6 $ $Date: 2005/01/20 02:25:49 $";
#endif 







#ifndef BASE_H
#include "base.h"
#endif 

#ifndef PKI1_H
#include "pki1.h"
#endif 

#include "plhash.h"
#include "plstr.h"





























const NSSOID *NSS_OID_UNKNOWN = (NSSOID *)NULL;





















NSS_EXTERN NSSOID *
NSSOID_CreateFromBER
(
  NSSBER *berOid
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  




  if( (NSSBER *)NULL == berOid ) {
    nss_SetError(NSS_ERROR_INVALID_BER);
    return (NSSOID *)NULL;
  }

  if( (void *)NULL == berOid->data ) {
    nss_SetError(NSS_ERROR_INVALID_BER);
    return (NSSOID *)NULL;
  }
#endif 
  
  return nssOID_CreateFromBER(berOid);
}


















NSS_EXTERN NSSOID *
NSSOID_CreateFromUTF8
(
  NSSUTF8 *stringOid
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  




  if( (NSSUTF8 *)NULL == stringOid ) {
    nss_SetError(NSS_ERROR_INVALID_UTF8);
    return (NSSOID *)NULL;
  }
#endif 

  return nssOID_CreateFromUTF8(stringOid);
}



















NSS_EXTERN NSSDER *
NSSOID_GetDEREncoding
(
  const NSSOID *oid,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  if( PR_SUCCESS != nssOID_verifyPointer(oid) ) {
    return (NSSDER *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSDER *)NULL;
    }
  }
#endif 

  return nssOID_GetDEREncoding(oid, rvOpt, arenaOpt);
}





















NSS_EXTERN NSSUTF8 *
NSSOID_GetUTF8Encoding
(
  const NSSOID *oid,
  NSSArena *arenaOpt
)
{
  nss_ClearErrorStack();

#ifdef DEBUG
  if( PR_SUCCESS != nssOID_verifyPointer(oid) ) {
    return (NSSUTF8 *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSUTF8 *)NULL;
    }
  }
#endif 

  return nssOID_GetUTF8Encoding(oid, arenaOpt);
}











static PLHashTable *oid_hash_table;





static PZLock *oid_hash_lock;








static PLHashNumber PR_CALLBACK
oid_hash
(
  const void *key
)
{
  const NSSItem *item = (const NSSItem *)key;
  PLHashNumber rv = 0;

  PRUint8 *data = (PRUint8 *)item->data;
  PRUint32 i;
  PRUint8 *rvc = (PRUint8 *)&rv;

  for( i = 0; i < item->size; i++ ) {
    rvc[ i % sizeof(rv) ] ^= *data;
    data++;
  }

  return rv;
}








static PRIntn PR_CALLBACK
oid_hash_compare
(
  const void *k1,
  const void *k2
)
{
  PRIntn rv;

  const NSSItem *i1 = (const NSSItem *)k1;
  const NSSItem *i2 = (const NSSItem *)k2;

  PRUint32 size = (i1->size < i2->size) ? i1->size : i2->size;

  rv = (PRIntn)nsslibc_memequal(i1->data, i2->data, size, (PRStatus *)NULL);
  if( 0 == rv ) {
    rv = i1->size - i2->size;
  }

  return !rv;
}





#ifdef DEBUG
extern const NSSError NSS_ERROR_INTERNAL_ERROR;

static nssPointerTracker oid_pointer_tracker;

static PRStatus
oid_add_pointer
(
  const NSSOID *oid
)
{
  PRStatus rv;

  rv = nssPointerTracker_initialize(&oid_pointer_tracker);
  if( PR_SUCCESS != rv ) {
    return rv;
  }

  rv = nssPointerTracker_add(&oid_pointer_tracker, oid);
  if( PR_SUCCESS != rv ) {
    NSSError e = NSS_GetError();
    if( NSS_ERROR_NO_MEMORY != e ) {
      nss_SetError(NSS_ERROR_INTERNAL_ERROR);
    }

    return rv;
  }

  return PR_SUCCESS;
}

#if defined(CAN_DELETE_OIDS)






static PRStatus
oid_remove_pointer
(
  const NSSOID *oid
)
{
  PRStatus rv;

  rv = nssPointerTracker_remove(&oid_pointer_tracker, oid);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INTERNAL_ERROR);
  }

  return rv;
}
#endif 

#endif 







static NSSArena *oid_arena;







static PRStatus PR_CALLBACK
oid_once_func
(
  void
)
{
  PRUint32 i;
  
  
  oid_arena = nssArena_Create();
  if( (NSSArena *)NULL == oid_arena ) {
    goto loser;
  }

  
  oid_hash_lock = PZ_NewLock(nssILockOID);
  if( (PZLock *)NULL == oid_hash_lock ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    goto loser;
  }

  
  oid_hash_table = PL_NewHashTable(0, oid_hash, oid_hash_compare,
                                   PL_CompareValues, 
                                   (PLHashAllocOps *)0,
                                   (void *)0);
  if( (PLHashTable *)NULL == oid_hash_table ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    goto loser;
  }

  
  for( i = 0; i < nss_builtin_oid_count; i++ ) {
    NSSOID *oid = (NSSOID *)&nss_builtin_oids[i];
    PLHashEntry *e = PL_HashTableAdd(oid_hash_table, &oid->data, oid);
    if( (PLHashEntry *)NULL == e ) {
      nss_SetError(NSS_ERROR_NO_MEMORY);
      goto loser;
    }

#ifdef DEBUG
    if( PR_SUCCESS != oid_add_pointer(oid) ) {
      goto loser;
    }
#endif 
  }

  return PR_SUCCESS;

 loser:
  if( (PLHashTable *)NULL != oid_hash_table ) {
    PL_HashTableDestroy(oid_hash_table);
    oid_hash_table = (PLHashTable *)NULL;
  }

  if( (PZLock *)NULL != oid_hash_lock ) {
    PZ_DestroyLock(oid_hash_lock);
    oid_hash_lock = (PZLock *)NULL;
  }

  if( (NSSArena *)NULL != oid_arena ) {
    (void)nssArena_Destroy(oid_arena);
    oid_arena = (NSSArena *)NULL;
  }

  return PR_FAILURE;
}





static PRCallOnceType oid_call_once;






static PRStatus
oid_init
(
  void
)
{
  return PR_CallOnce(&oid_call_once, oid_once_func);
}

#ifdef DEBUG



















NSS_EXTERN PRStatus
nssOID_verifyPointer
(
  const NSSOID *oid
)
{
  PRStatus rv;

  rv = oid_init();
  if( PR_SUCCESS != rv ) {
    return PR_FAILURE;
  }

  rv = nssPointerTracker_initialize(&oid_pointer_tracker);
  if( PR_SUCCESS != rv ) {
    return PR_FAILURE;
  }

  rv = nssPointerTracker_verify(&oid_pointer_tracker, oid);
  if( PR_SUCCESS != rv ) {
    nss_SetError(NSS_ERROR_INVALID_NSSOID);
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}
#endif 








static PRStatus
oid_sanity_check_ber
(
  NSSBER *berOid
)
{
  PRUint32 i;
  PRUint8 *data = (PRUint8 *)berOid->data;

  



  if( berOid->size <= 0 ) {
    return PR_FAILURE;
  }

  

















  if( data[0] >= 0x80 ) {
    return PR_FAILURE;
  }

  








  for( i = 1; i < berOid->size; i++ ) {
    if( (0x80 == data[i]) && (data[i-1] < 0x80) ) {
      return PR_FAILURE;
    }
  }

  





  if( data[ berOid->size-1 ] >= 0x80 ) {
    return PR_FAILURE;
  }

  


  return PR_SUCCESS;
}

















NSS_EXTERN NSSOID *
nssOID_CreateFromBER
(
  NSSBER *berOid
)
{
  NSSOID *rv;
  PLHashEntry *e;
  
  if( PR_SUCCESS != oid_init() ) {
    return (NSSOID *)NULL;
  }

  if( PR_SUCCESS != oid_sanity_check_ber(berOid) ) {
    nss_SetError(NSS_ERROR_INVALID_BER);
    return (NSSOID *)NULL;
  }

  


  PZ_Lock(oid_hash_lock);
  rv = (NSSOID *)PL_HashTableLookup(oid_hash_table, berOid);
  (void)PZ_Unlock(oid_hash_lock);
  if( (NSSOID *)NULL != rv ) {
    
    return rv;
  }

  


  rv = nss_ZNEW(oid_arena, NSSOID);
  if( (NSSOID *)NULL == rv ) {
    return (NSSOID *)NULL;
  }

  rv->data.data = nss_ZAlloc(oid_arena, berOid->size);
  if( (void *)NULL == rv->data.data ) {
    return (NSSOID *)NULL;
  }

  rv->data.size = berOid->size;
  nsslibc_memcpy(rv->data.data, berOid->data, berOid->size);

#ifdef DEBUG
  rv->tag = "<runtime>";
  rv->expl = "(OID registered at runtime)";
#endif 

  PZ_Lock(oid_hash_lock);
  e = PL_HashTableAdd(oid_hash_table, &rv->data, rv);
  (void)PZ_Unlock(oid_hash_lock);
  if( (PLHashEntry *)NULL == e ) {
    nss_ZFreeIf(rv->data.data);
    nss_ZFreeIf(rv);
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (NSSOID *)NULL;
  }

#ifdef DEBUG
  {
    PRStatus st;
    st = oid_add_pointer(rv);
    if( PR_SUCCESS != st ) {
      PZ_Lock(oid_hash_lock);
      (void)PL_HashTableRemove(oid_hash_table, &rv->data);
      (void)PZ_Unlock(oid_hash_lock);
      (void)nss_ZFreeIf(rv->data.data);
      (void)nss_ZFreeIf(rv);
      return (NSSOID *)NULL;
    }
  }
#endif 

  return rv;
}








static PRStatus
oid_sanity_check_utf8
(
  NSSUTF8 *s
)
{
  



  if( '#' == *s ) {
    s++;
  }

  



  if( (*s < '0') || (*s > '9') ) {
    return PR_FAILURE;
  }

  





  if( (s[1] != '.') && (s[1] != '\0') ) {
    return PR_FAILURE;
  }

  



  for( ; '\0' != *s; s++ ) {
    if( ('.' != *s) && ((*s < '0') || (*s > '9')) ) {
      return PR_FAILURE;
    }

    
    if( ('.' == *s) && ('.' == s[1]) ) {
      return PR_FAILURE;
    }
  }

  



  if( '.' == *--s ) {
    return PR_FAILURE;
  }

  return PR_SUCCESS;
}

static PRUint32
oid_encode_number
(
  PRUint32 n,
  PRUint8 *dp,
  PRUint32 nb
)
{
  PRUint32 a[5];
  PRUint32 i;
  PRUint32 rv;

  a[0] = (n >> 28) & 0x7f;
  a[1] = (n >> 21) & 0x7f;
  a[2] = (n >> 14) & 0x7f;
  a[3] = (n >>  7) & 0x7f;
  a[4] =  n        & 0x7f;

  for( i = 0; i < 5; i++ ) {
    if( 0 != a[i] ) {
      break;
    }
  }

  if( 5 == i ) {
    i--;
  }

  rv = 5-i;
  if( rv > nb ) {
    return rv;
  }

  for( ; i < 4; i++ ) {
    *dp = 0x80 | a[i];
    dp++;
  }

  *dp = a[4];

  return rv;
}











static PRUint32
oid_encode_huge
(
  NSSUTF8 *s,
  NSSUTF8 *e,
  PRUint8 *dp,
  PRUint32 nb
)
{
  PRUint32 slen = (e-s);
  PRUint32 blen = (slen+1)/2;
  PRUint8 *st = (PRUint8 *)NULL;
  PRUint8 *bd = (PRUint8 *)NULL;
  PRUint32 i;
  PRUint32 bitno;
  PRUint8 *last;
  PRUint8 *first;
  PRUint32 byteno;
  PRUint8 mask;

  
  st = (PRUint8 *)nss_ZAlloc((NSSArena *)NULL, slen);
  if( (PRUint8 *)NULL == st ) {
    return 0;
  }

  
  bd = (PRUint8 *)nss_ZAlloc((NSSArena *)NULL, blen);
  if( (PRUint8 *)NULL == bd ) {
    (void)nss_ZFreeIf(st);
    return 0;
  }

  
  for( i = 0; i < slen; i++ ) {
    st[i] = (PRUint8)(s[i] - '0');
  }

  last = &st[slen-1];
  first = &st[0];

  






  for( bitno = 0; ; bitno++ ) {
    PRUint8 *d;

    byteno = bitno/7;
    mask = (PRUint8)(1 << (bitno%7));

    
    for( ; first < last; first ++ ) {
      if( 0 != *first ) {
        break;
      }
    }

    
    if( (first == last) && (0 == *last) ) {
      break;
    }

    
    if( *last & 1 ) {
      bd[ byteno ] |= mask;
    }


    






    *last /= 2;

    for( d = &last[-1]; d >= first; d-- ) {
      if( *d & 1 ) {
        d[1] += 5;
      }

      *d /= 2;
    }
  }

  
  if( (byteno+1) > nb ) {
    return (byteno+1);
  }

  
  for( ; byteno > 0; byteno-- ) {
    if( 0 != bd[ byteno ] ) {
      break;
    }
  }

  
  for( i = 0; i < byteno; i++ ) {
    dp[i] = bd[ byteno-i ] | 0x80;
  }
  
  dp[byteno] = bd[0];

  (void)nss_ZFreeIf(bd);
  (void)nss_ZFreeIf(st);
  return (byteno+1);
}








extern const NSSError NSS_ERROR_INTERNAL_ERROR;

static NSSOID *
oid_encode_string
(
  NSSUTF8 *s
)
{
  PRUint32 nn = 0; 
  PRUint32 nb = 0; 
  NSSUTF8 *t;
  PRUint32 nd = 0; 
  NSSOID *rv;
  PRUint8 *dp;
  PRUint32 a, b;
  PRUint32 inc;

  
  if( '#' == *s ) {
    s++;
  }

  
  for( t = s; '\0' != *t; t++ ) {
    if( '.' == *t ) {
      nb += (nd+1)/2; 
      nd = 0;
      nn++;
    } else {
      nd++;
    }
  }
  nb += (nd+1)/2;
  nn++;

  if( 1 == nn ) {
    



    nb++;
  }

  







  rv = nss_ZNEW((NSSArena *)NULL, NSSOID);
  if( (NSSOID *)NULL == rv ) {
    return (NSSOID *)NULL;
  }

  rv->data.data = nss_ZAlloc((NSSArena *)NULL, nb);
  if( (void *)NULL == rv->data.data ) {
    (void)nss_ZFreeIf(rv);
    return (NSSOID *)NULL;
  }

  dp = (PRUint8 *)rv->data.data;

  a = atoi(s);

  if( 1 == nn ) {
    dp[0] = '\x80';
    inc = oid_encode_number(a, &dp[1], nb-1);
    if( inc >= nb ) {
      goto loser;
    }
  } else {
    for( t = s; '.' != *t; t++ ) {
      ;
    }

    t++;
    b = atoi(t);
    inc = oid_encode_number(a*40+b, dp, nb);
    if( inc > nb ) {
      goto loser;
    }
    dp += inc;
    nb -= inc;
    nn -= 2;

    while( nn-- > 0 ) {
      NSSUTF8 *u;

      for( ; '.' != *t; t++ ) {
        ;
      }

      t++;

      for( u = t; ('\0' != *u) && ('.' != *u); u++ ) {
        ;
      }

      if( (u-t > 9) ) {
        
        inc = oid_encode_huge(t, u, dp, nb);
      } else {
        b = atoi(t);
        inc = oid_encode_number(b, dp, nb);
      }

      if( inc > nb ) {
        goto loser;
      }
      dp += inc;
      nb -= inc;
    }
  }

  return rv;

 loser:
  nss_SetError(NSS_ERROR_INTERNAL_ERROR);
  return (NSSOID *)NULL;
}



















NSS_EXTERN NSSOID *
nssOID_CreateFromUTF8
(
  NSSUTF8 *stringOid
)
{
  NSSOID *rv = (NSSOID *)NULL;
  NSSOID *candidate = (NSSOID *)NULL;
  PLHashEntry *e;

  if( PR_SUCCESS != oid_init() ) {
    return (NSSOID *)NULL;
  }

  if( PR_SUCCESS != oid_sanity_check_utf8(stringOid) ) {
    nss_SetError(NSS_ERROR_INVALID_STRING);
    return (NSSOID *)NULL;
  }

  candidate = oid_encode_string(stringOid);
  if( (NSSOID *)NULL == candidate ) {
    
    return rv;
  }

  


  PZ_Lock(oid_hash_lock);
  rv = (NSSOID *)PL_HashTableLookup(oid_hash_table, &candidate->data);
  (void)PZ_Unlock(oid_hash_lock);
  if( (NSSOID *)NULL != rv ) {
    
    (void)nss_ZFreeIf(candidate->data.data);
    (void)nss_ZFreeIf(candidate);
    return rv;
  }

  



  rv = nss_ZNEW(oid_arena, NSSOID);
  if( (NSSOID *)NULL == rv ) {
    goto loser;
  }

  rv->data.data = nss_ZAlloc(oid_arena, candidate->data.size);
  if( (void *)NULL == rv->data.data ) {
    goto loser;
  }

  rv->data.size = candidate->data.size;
  nsslibc_memcpy(rv->data.data, candidate->data.data, rv->data.size);

  (void)nss_ZFreeIf(candidate->data.data);
  (void)nss_ZFreeIf(candidate);

#ifdef DEBUG
  rv->tag = "<runtime>";
  rv->expl = "(OID registered at runtime)";
#endif 

  PZ_Lock(oid_hash_lock);
  e = PL_HashTableAdd(oid_hash_table, &rv->data, rv);
  (void)PZ_Unlock(oid_hash_lock);
  if( (PLHashEntry *)NULL == e ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    goto loser;
  }

#ifdef DEBUG
  {
    PRStatus st;
    st = oid_add_pointer(rv);
    if( PR_SUCCESS != st ) {
      PZ_Lock(oid_hash_lock);
      (void)PL_HashTableRemove(oid_hash_table, &rv->data);
      (void)PZ_Unlock(oid_hash_lock);
      goto loser;
    }
  }
#endif 

  return rv;

 loser:
  if( (NSSOID *)NULL != candidate ) {
    (void)nss_ZFreeIf(candidate->data.data);
  }
  (void)nss_ZFreeIf(candidate);

  if( (NSSOID *)NULL != rv ) {
    (void)nss_ZFreeIf(rv->data.data);
  }
  (void)nss_ZFreeIf(rv);

  return (NSSOID *)NULL;
}



















NSS_EXTERN NSSDER *
nssOID_GetDEREncoding
(
  const NSSOID *oid,
  NSSDER *rvOpt,
  NSSArena *arenaOpt
)
{
  const NSSItem *it;
  NSSDER *rv;

  if( PR_SUCCESS != oid_init() ) {
    return (NSSDER *)NULL;
  }

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssOID_verifyPointer(oid) ) {
    return (NSSDER *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSDER *)NULL;
    }
  }
#endif 

  it = &oid->data;

  if( (NSSDER *)NULL == rvOpt ) {
    rv = nss_ZNEW(arenaOpt, NSSDER);
    if( (NSSDER *)NULL == rv ) {
      return (NSSDER *)NULL;
    }
  } else {
    rv = rvOpt;
  }

  rv->data = nss_ZAlloc(arenaOpt, it->size);
  if( (void *)NULL == rv->data ) {
    if( rv != rvOpt ) {
      (void)nss_ZFreeIf(rv);
    }
    return (NSSDER *)NULL;
  }

  rv->size = it->size;
  nsslibc_memcpy(rv->data, it->data, it->size);

  return rv;
}





















NSS_EXTERN NSSUTF8 *
nssOID_GetUTF8Encoding
(
  const NSSOID *oid,
  NSSArena *arenaOpt
)
{
  NSSUTF8 *rv;
  PRUint8 *end;
  PRUint8 *d;
  PRUint8 *e;
  char *a;
  char *b;
  PRUint32 len;

  if( PR_SUCCESS != oid_init() ) {
    return (NSSUTF8 *)NULL;
  }

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssOID_verifyPointer(oid) ) {
    return (NSSUTF8 *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSUTF8 *)NULL;
    }
  }
#endif 

  a = (char *)NULL;

  
  d = (PRUint8 *)oid->data.data;
  
  end = &d[ oid->data.size ];

#ifdef NSSDEBUG
  







  if( end[-1] & 0x80 ) {
    nss_SetError(NSS_ERROR_INTERNAL_ERROR);
    return (NSSUTF8 *)NULL;
  }
#endif 

  


  if( (*d == 0x80) && (2 == oid->data.size) ) {
    
    a = PR_smprintf("%lu", (PRUint32)d[1]);
    if( (char *)NULL == a ) {
      nss_SetError(NSS_ERROR_NO_MEMORY);
      return (NSSUTF8 *)NULL;
    }
    goto done;
  }

  for( ; d < end; d = &e[1] ) {
    
    for( e = d; e < end; e++ ) {
      if( 0 == (*e & 0x80) ) {
        break;
      }
    }
    
    if( ((e-d) > 4) || (((e-d) == 4) && (*d & 0x70)) ) {
      
    } else {
      PRUint32 n = 0;
      
      switch( e-d ) {
      case 4:
        n |= ((PRUint32)(e[-4] & 0x0f)) << 28;
      case 3:
        n |= ((PRUint32)(e[-3] & 0x7f)) << 21;
      case 2:
        n |= ((PRUint32)(e[-2] & 0x7f)) << 14;
      case 1:
        n |= ((PRUint32)(e[-1] & 0x7f)) <<  7;
      case 0:
        n |= ((PRUint32)(e[-0] & 0x7f))      ;
      }
      
      if( (char *)NULL == a ) {
        
        PRUint32 one = (n/40), two = (n%40);
        
        a = PR_smprintf("%lu.%lu", one, two);
        if( (char *)NULL == a ) {
          nss_SetError(NSS_ERROR_NO_MEMORY);
          return (NSSUTF8 *)NULL;
        }
      } else {
        b = PR_smprintf("%s.%lu", a, n);
        if( (char *)NULL == b ) {
          PR_smprintf_free(a);
          nss_SetError(NSS_ERROR_NO_MEMORY);
          return (NSSUTF8 *)NULL;
        }
        
        PR_smprintf_free(a);
        a = b;
      }
    }
  }

 done:
  




  len = PL_strlen(a);
  rv = (NSSUTF8 *)nss_ZAlloc(arenaOpt, len);
  if( (NSSUTF8 *)NULL == rv ) {
    PR_smprintf_free(a);
    return (NSSUTF8 *)NULL;
  }

  nsslibc_memcpy(rv, a, len);
  PR_smprintf_free(a);

  return rv;
}

























#ifdef DEBUG
NSS_EXTERN const NSSUTF8 *
nssOID_getExplanation
(
  NSSOID *oid
)
{
  if( PR_SUCCESS != oid_init() ) {
    return (const NSSUTF8 *)NULL;
  }

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssOID_verifyPointer(oid) ) {
    return (NSSUTF8 *)NULL;
  }
#endif 

  return oid->expl;
}

extern const NSSError NSS_ERROR_INVALID_NSSOID;
#endif 



























#ifdef DEBUG
NSS_EXTERN NSSUTF8 *
nssOID_getTaggedUTF8
(
  NSSOID *oid,
  NSSArena *arenaOpt
)
{
  NSSUTF8 *rv;
  char *raw;
  char *c;
  char *a = (char *)NULL;
  char *b;
  PRBool done = PR_FALSE;
  PRUint32 len;

  if( PR_SUCCESS != oid_init() ) {
    return (NSSUTF8 *)NULL;
  }

#ifdef NSSDEBUG
  if( PR_SUCCESS != nssOID_verifyPointer(oid) ) {
    return (NSSUTF8 *)NULL;
  }

  if( (NSSArena *)NULL != arenaOpt ) {
    if( PR_SUCCESS != nssArena_verifyPointer(arenaOpt) ) {
      return (NSSUTF8 *)NULL;
    }
  }
#endif 

  a = PR_smprintf("{");
  if( (char *)NULL == a ) {
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (NSSUTF8 *)NULL;
  }

  









  
  raw = (char *)nssOID_GetUTF8Encoding(oid, (NSSArena *)NULL);
  if( (char *)NULL == raw ) {
    return (NSSUTF8 *)NULL;
  }

  for( c = raw; !done; c++ ) {
    NSSOID *lead;
    char *lastdot;

    for( ; '.' != *c; c++ ) {
      if( '\0' == *c ) {
        done = PR_TRUE;
        break;
      }
    }

    *c = '\0';
    lead = nssOID_CreateFromUTF8((NSSUTF8 *)raw);
    if( (NSSOID *)NULL == lead ) {
      PR_smprintf_free(a);
      nss_ZFreeIf(raw);
      nss_SetError(NSS_ERROR_NO_MEMORY);
      return (NSSUTF8 *)NULL;
    }

    lastdot = PL_strrchr(raw, '.');
    if( (char *)NULL == lastdot ) {
      lastdot = raw;
    }

    b = PR_smprintf("%s %s(%s) ", a, lead->tag, &lastdot[1]);
    if( (char *)NULL == b ) {
      PR_smprintf_free(a);
      nss_ZFreeIf(raw);
      
      nss_SetError(NSS_ERROR_NO_MEMORY);
      return (NSSUTF8 *)NULL;
    }

    PR_smprintf_free(a);
    a = b;

    if( !done ) {
      *c = '.';
    }
  }

  nss_ZFreeIf(raw);

  b = PR_smprintf("%s }", a);
  if( (char *)NULL == b ) {
    PR_smprintf_free(a);
    nss_SetError(NSS_ERROR_NO_MEMORY);
    return (NSSUTF8 *)NULL;
  }

  len = PL_strlen(b);
  
  rv = (NSSUTF8 *)nss_ZAlloc(arenaOpt, len+1);
  if( (NSSUTF8 *)NULL == rv ) {
    PR_smprintf_free(b);
    return (NSSUTF8 *)NULL;
  }

  nsslibc_memcpy(rv, b, len);
  PR_smprintf_free(b);

  return rv;
}

extern const NSSError NSS_ERROR_INVALID_NSSOID;
extern const NSSError NSS_ERROR_NO_MEMORY;
#endif 
