









































#ifndef prmem_h___
#define prmem_h___

#include "prtypes.h"
#include <stdlib.h>

PR_BEGIN_EXTERN_C
















NSPR_API(void *) PR_Malloc(PRUint32 size);

NSPR_API(void *) PR_Calloc(PRUint32 nelem, PRUint32 elsize);

NSPR_API(void *) PR_Realloc(void *ptr, PRUint32 size);

NSPR_API(void) PR_Free(void *ptr);














#define PR_MALLOC(_bytes) (PR_Malloc((_bytes)))









#define PR_NEW(_struct) ((_struct *) PR_MALLOC(sizeof(_struct)))











#define PR_REALLOC(_ptr, _size) (PR_Realloc((_ptr), (_size)))










#define PR_CALLOC(_size) (PR_Calloc(1, (_size)))










#define PR_NEWZAP(_struct) ((_struct*)PR_Calloc(1, sizeof(_struct)))










#define PR_DELETE(_ptr) { PR_Free(_ptr); (_ptr) = NULL; }











#define PR_FREEIF(_ptr)	if (_ptr) PR_DELETE(_ptr)

PR_END_EXTERN_C

#endif 
