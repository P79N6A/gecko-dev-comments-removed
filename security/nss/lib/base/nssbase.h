



#ifndef NSSBASE_H
#define NSSBASE_H








#ifndef NSSBASET_H
#include "nssbaset.h"
#endif 

PR_BEGIN_EXTERN_C



























NSS_EXTERN NSSArena *
NSSArena_Create
(
  void
);

extern const NSSError NSS_ERROR_NO_MEMORY;

















NSS_EXTERN PRStatus
NSSArena_Destroy
(
  NSSArena *arena
);

extern const NSSError NSS_ERROR_INVALID_ARENA;

























NSS_EXTERN NSSError
NSS_GetError
(
  void
);

extern const NSSError NSS_ERROR_NO_ERROR;



















NSS_EXTERN NSSError *
NSS_GetErrorStack
(
  void
);






















#define NSS_ZNEW(arenaOpt, type) ((type *)NSS_ZAlloc((arenaOpt), sizeof(type)))






















#define NSS_ZNEWARRAY(arenaOpt, type, quantity) ((type *)NSS_ZAlloc((arenaOpt), sizeof(type) * (quantity)))

























NSS_EXTERN void *
NSS_ZAlloc
(
  NSSArena *arenaOpt,
  PRUint32 size
);





















NSS_EXTERN void *
NSS_ZRealloc
(
  void *pointer,
  PRUint32 newSize
);




















NSS_EXTERN PRStatus
NSS_ZFreeIf
(
  void *pointer
);

PR_END_EXTERN_C

#endif 
