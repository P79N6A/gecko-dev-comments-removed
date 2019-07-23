










































#ifndef _PKIX_PL_OBJECT_H
#define _PKIX_PL_OBJECT_H

#include "pkix_pl_common.h"

#ifdef __cplusplus
extern "C" {
#endif





















struct PKIX_PL_ObjectStruct {
        PRUint64    magicHeader;
        PKIX_UInt32 type;
        PKIX_Int32 references;
        PRLock *lock;
        PKIX_PL_String *stringRep;
        PKIX_UInt32 hashcode;
        PKIX_Boolean hashcodeCached;
};



PKIX_Error *
pkix_pl_Object_RetrieveEqualsCallback(
        PKIX_PL_Object *object,
        PKIX_PL_EqualsCallback *equalsCallback,
        void *plContext);

extern PKIX_Boolean initializing;
extern PKIX_Boolean initialized;

#ifdef PKIX_USER_OBJECT_TYPE

extern PRLock *classTableLock;

#endif

extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];

PKIX_Error *
pkix_pl_Object_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
