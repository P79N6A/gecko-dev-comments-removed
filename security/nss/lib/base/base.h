



































#ifndef BASE_H
#define BASE_H

#ifdef DEBUG
static const char BASE_CVS_ID[] = "@(#) $RCSfile: base.h,v $ $Revision: 1.20 $ $Date: 2008/05/10 01:03:14 $";
#endif 








#ifndef BASET_H
#include "baset.h"
#endif 

#ifndef NSSBASE_H
#include "nssbase.h"
#endif 

#include "plhash.h"

PR_BEGIN_EXTERN_C




























































NSS_EXTERN NSSArena *
nssArena_Create
(
  void
);

extern const NSSError NSS_ERROR_NO_MEMORY;

















NSS_EXTERN PRStatus
nssArena_Destroy
(
  NSSArena *arena
);

extern const NSSError NSS_ERROR_INVALID_ARENA;






















NSS_EXTERN nssArenaMark *
nssArena_Mark
(
  NSSArena *arena
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_NO_MEMORY;
extern const NSSError NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD;




















NSS_EXTERN PRStatus
nssArena_Release
(
  NSSArena *arena,
  nssArenaMark *arenaMark
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_INVALID_ARENA_MARK;























NSS_EXTERN PRStatus
nssArena_Unmark
(
  NSSArena *arena,
  nssArenaMark *arenaMark
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_INVALID_ARENA_MARK;
extern const NSSError NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD;

#ifdef ARENA_DESTRUCTOR_LIST



























NSS_EXTERN PRStatus
nssArena_registerDestructor
(
  NSSArena *arena,
  void (*destructor)(void *argument),
  void *arg
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_NO_MEMORY;




















NSS_EXTERN PRStatus
nssArena_deregisterDestructor
(
  NSSArena *arena,
  void (*destructor)(void *argument),
  void *arg
);

extern const NSSError NSS_ERROR_INVALID_ITEM;
extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_NOT_FOUND;

#endif 
























NSS_EXTERN void *
nss_ZAlloc
(
  NSSArena *arenaOpt,
  PRUint32 size
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
extern const NSSError NSS_ERROR_NO_MEMORY;
extern const NSSError NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD;



















NSS_EXTERN PRStatus
nss_ZFreeIf
(
  void *pointer
);

extern const NSSError NSS_ERROR_INVALID_POINTER;





















NSS_EXTERN void *
nss_ZRealloc
(
  void *pointer,
  PRUint32 newSize
);

extern const NSSError NSS_ERROR_INVALID_POINTER;
extern const NSSError NSS_ERROR_NO_MEMORY;
extern const NSSError NSS_ERROR_ARENA_MARKED_BY_ANOTHER_THREAD;






















#define nss_ZNEW(arenaOpt, type) ((type *)nss_ZAlloc((arenaOpt), sizeof(type)))






















#define nss_ZNEWARRAY(arenaOpt, type, quantity) ((type *)nss_ZAlloc((arenaOpt), sizeof(type) * (quantity)))


















#define nss_ZREALLOCARRAY(p, type, quantity) ((type *)nss_ZRealloc((p), sizeof(type) * (quantity)))


















#ifdef DEBUG
NSS_EXTERN PRStatus
nssArena_verifyPointer
(
  const NSSArena *arena
);

extern const NSSError NSS_ERROR_INVALID_ARENA;
#endif 














#ifdef DEBUG
#define nssArena_VERIFYPOINTER(p) nssArena_verifyPointer(p)
#else 

#define nssArena_VERIFYPOINTER(p) (((NSSArena *)NULL == (p))?PR_FAILURE:PR_SUCCESS)
#endif 





extern PRStatus
nssArena_Shutdown(void);












NSS_EXTERN_DATA PLHashAllocOps nssArenaHashAllocOps;


















NSS_EXTERN void
nss_SetError
(
  PRUint32 error
);







NSS_EXTERN void
nss_ClearErrorStack
(
  void
);







NSS_EXTERN void
nss_DestroyErrorStack
(
  void
);









NSS_EXTERN NSSItem *
nssItem_Create
(
  NSSArena *arenaOpt,
  NSSItem *rvOpt,
  PRUint32 length,
  const void *data
);

NSS_EXTERN void
nssItem_Destroy
(
  NSSItem *item
);

NSS_EXTERN NSSItem *
nssItem_Duplicate
(
  NSSItem *obj,
  NSSArena *arenaOpt,
  NSSItem *rvOpt
);

NSS_EXTERN PRBool
nssItem_Equal
(
  const NSSItem *one,
  const NSSItem *two,
  PRStatus *statusOpt
);


























NSS_EXTERN PRBool
nssUTF8_CaseIgnoreMatch
(
  const NSSUTF8 *a,
  const NSSUTF8 *b,
  PRStatus *statusOpt
);


















NSS_EXTERN NSSUTF8 *
nssUTF8_Duplicate
(
  const NSSUTF8 *s,
  NSSArena *arenaOpt
);























NSS_EXTERN PRBool
nssUTF8_PrintableMatch
(
  const NSSUTF8 *a,
  const NSSUTF8 *b,
  PRStatus *statusOpt
);

















NSS_EXTERN PRUint32
nssUTF8_Size
(
  const NSSUTF8 *s,
  PRStatus *statusOpt
);

extern const NSSError NSS_ERROR_INVALID_POINTER;
extern const NSSError NSS_ERROR_VALUE_TOO_LARGE;


















NSS_EXTERN PRUint32
nssUTF8_Length
(
  const NSSUTF8 *s,
  PRStatus *statusOpt
);

extern const NSSError NSS_ERROR_INVALID_POINTER;
extern const NSSError NSS_ERROR_VALUE_TOO_LARGE;
extern const NSSError NSS_ERROR_INVALID_STRING;
























NSS_EXTERN NSSUTF8 *
nssUTF8_Create
(
  NSSArena *arenaOpt,
  nssStringType type,
  const void *inputString,
  PRUint32 size 
);

extern const NSSError NSS_ERROR_INVALID_POINTER;
extern const NSSError NSS_ERROR_NO_MEMORY;
extern const NSSError NSS_ERROR_UNSUPPORTED_TYPE;

NSS_EXTERN NSSItem *
nssUTF8_GetEncoding
(
  NSSArena *arenaOpt,
  NSSItem *rvOpt,
  nssStringType type,
  NSSUTF8 *string
);












extern const NSSError NSS_ERROR_INVALID_POINTER;
extern const NSSError NSS_ERROR_INVALID_ARGUMENT;

NSS_EXTERN PRStatus
nssUTF8_CopyIntoFixedBuffer
(
  NSSUTF8 *string,
  char *buffer,
  PRUint32 bufferSize,
  char pad
);






NSS_EXTERN PRBool
nssUTF8_Equal
(
  const NSSUTF8 *a,
  const NSSUTF8 *b,
  PRStatus *statusOpt
);
















NSS_EXTERN nssList *
nssList_Create
(
  NSSArena *arenaOpt,
  PRBool threadSafe
);




NSS_EXTERN PRStatus
nssList_Destroy
(
  nssList *list
);

NSS_EXTERN void
nssList_Clear
(
  nssList *list, 
  nssListElementDestructorFunc destructor
);








NSS_EXTERN void
nssList_SetCompareFunction
(
  nssList *list, 
  nssListCompareFunc compareFunc
);






NSS_EXTERN void
nssList_SetSortFunction
(
  nssList *list, 
  nssListSortFunc sortFunc
);




NSS_EXTERN PRStatus
nssList_Add
(
  nssList *list, 
  void *data
);







NSS_EXTERN PRStatus
nssList_AddUnique
(
  nssList *list, 
  void *data
);






NSS_EXTERN PRStatus
nssList_Remove(nssList *list, void *data);







NSS_EXTERN void *
nssList_Get
(
  nssList *list, 
  void *data
);




NSS_EXTERN PRUint32
nssList_Count
(
  nssList *list
);







NSS_EXTERN PRStatus
nssList_GetArray
(
  nssList *list, 
  void **rvArray, 
  PRUint32 maxElements
);






NSS_EXTERN nssListIterator *
nssList_CreateIterator
(
  nssList *list
);

NSS_EXTERN nssList *
nssList_Clone
(
  nssList *list
);




NSS_EXTERN void
nssListIterator_Destroy
(
  nssListIterator *iter
);







NSS_EXTERN void *
nssListIterator_Start
(
  nssListIterator *iter
);






NSS_EXTERN void *
nssListIterator_Next
(
  nssListIterator *iter
);







NSS_EXTERN PRStatus
nssListIterator_Finish
(
  nssListIterator *iter
);



















NSS_EXTERN nssHash *
nssHash_Create
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets,
  PLHashFunction keyHash,
  PLHashComparator keyCompare,
  PLHashComparator valueCompare
);

NSS_EXTERN nssHash *
nssHash_CreatePointer
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
);

NSS_EXTERN nssHash *
nssHash_CreateString
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
);

NSS_EXTERN nssHash *
nssHash_CreateItem
(
  NSSArena *arenaOpt,
  PRUint32 numBuckets
);





NSS_EXTERN void
nssHash_Destroy
(
  nssHash *hash
);






extern const NSSError NSS_ERROR_HASH_COLLISION;

NSS_EXTERN PRStatus
nssHash_Add
(
  nssHash *hash,
  const void *key,
  const void *value
);





NSS_EXTERN void
nssHash_Remove
(
  nssHash *hash,
  const void *it
);





NSS_EXTERN PRUint32
nssHash_Count
(
  nssHash *hash
);





NSS_EXTERN PRBool
nssHash_Exists
(
  nssHash *hash,
  const void *it
);





NSS_EXTERN void *
nssHash_Lookup
(
  nssHash *hash,
  const void *it
);





NSS_EXTERN void
nssHash_Iterate
(
  nssHash *hash,
  nssHashIterator fcn,
  void *closure
);





































#ifdef DEBUG
NSS_EXTERN PRStatus
nssPointerTracker_initialize
(
  nssPointerTracker *tracker
);

extern const NSSError NSS_ERROR_NO_MEMORY;
#endif 
























#ifdef DEBUG
NSS_EXTERN PRStatus
nssPointerTracker_finalize
(
  nssPointerTracker *tracker
);

extern const NSSError NSS_ERROR_TRACKER_NOT_EMPTY;
#endif 























#ifdef DEBUG
NSS_EXTERN PRStatus
nssPointerTracker_add
(
  nssPointerTracker *tracker,
  const void *pointer
);

extern const NSSError NSS_ERROR_NO_MEMORY;
extern const NSSError NSS_ERROR_TRACKER_NOT_INITIALIZED;
extern const NSSError NSS_ERROR_DUPLICATE_POINTER;
#endif 























#ifdef DEBUG
NSS_EXTERN PRStatus
nssPointerTracker_remove
(
  nssPointerTracker *tracker,
  const void *pointer
);

extern const NSSError NSS_ERROR_TRACKER_NOT_INITIALIZED;
extern const NSSError NSS_ERROR_POINTER_NOT_REGISTERED;
#endif 
























#ifdef DEBUG
NSS_EXTERN PRStatus
nssPointerTracker_verify
(
  nssPointerTracker *tracker,
  const void *pointer
);

extern const NSSError NSS_ERROR_POINTER_NOT_REGISTERED;
#endif 




















NSS_EXTERN void *
nsslibc_memcpy
(
  void *dest,
  const void *source,
  PRUint32 n
);

extern const NSSError NSS_ERROR_INVALID_POINTER;












NSS_EXTERN void *
nsslibc_memset
(
  void *dest,
  PRUint8 byte,
  PRUint32 n
);

extern const NSSError NSS_ERROR_INVALID_POINTER;













NSS_EXTERN PRBool
nsslibc_memequal
(
  const void *a,
  const void *b,
  PRUint32 len,
  PRStatus *statusOpt
);

extern const NSSError NSS_ERROR_INVALID_POINTER;

#define nsslibc_offsetof(str, memb) ((PRPtrdiff)(&(((str *)0)->memb)))

PR_END_EXTERN_C

#endif 
