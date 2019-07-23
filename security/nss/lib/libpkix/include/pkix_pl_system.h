









































#ifndef _PKIX_PL_SYSTEM_H
#define _PKIX_PL_SYSTEM_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif
































































PKIX_Error *
PKIX_PL_Initialize(
        PKIX_Boolean platformInitNeeded,
        PKIX_Boolean useArenas,
        void **pPlContext);



































PKIX_Error *
PKIX_PL_Shutdown(void *plContext);

























PKIX_Error *
PKIX_PL_Malloc(
        PKIX_UInt32 size,
        void **pMemory,
        void *plContext);



























PKIX_Error *
PKIX_PL_Calloc(
        PKIX_UInt32 nElem,
        PKIX_UInt32 elSize,
        void **pMemory,
        void *plContext);




























PKIX_Error *
PKIX_PL_Realloc(
        void *ptr,
        PKIX_UInt32 size,
        void **pNewPtr,
        void *plContext);



















PKIX_Error *
PKIX_PL_Free(
        void *ptr,
        void *plContext);



































typedef PKIX_Error *
(*PKIX_PL_DestructorCallback)(
        PKIX_PL_Object *object,
        void *plContext);




























typedef PKIX_Error *
(*PKIX_PL_EqualsCallback)(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext);

























typedef PKIX_Error *
(*PKIX_PL_HashcodeCallback)(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pValue,
        void *plContext);

























typedef PKIX_Error *
(*PKIX_PL_ToStringCallback)(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext);
































typedef PKIX_Error *
(*PKIX_PL_ComparatorCallback)(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext);






























typedef PKIX_Error *
(*PKIX_PL_DuplicateCallback)(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext);

































PKIX_Error *
PKIX_PL_Object_Alloc(
        PKIX_TYPENUM type,
        PKIX_UInt32 size,
        PKIX_PL_Object **pObject,
        void *plContext);























PKIX_Error *
PKIX_PL_Object_IsTypeRegistered(
        PKIX_UInt32 type,
        PKIX_Boolean *pBool,
        void *plContext);

#ifdef PKIX_USER_OBJECT_TYPE













































PKIX_Error *
PKIX_PL_Object_RegisterType(
        PKIX_UInt32 type,
        char *description,
        PKIX_PL_DestructorCallback destructor,
        PKIX_PL_EqualsCallback equalsFunction,
        PKIX_PL_HashcodeCallback hashcodeFunction,
        PKIX_PL_ToStringCallback toStringFunction,
        PKIX_PL_ComparatorCallback comparator,
        PKIX_PL_DuplicateCallback duplicateFunction,
        void *plContext);

#endif
































PKIX_Error *
PKIX_PL_Object_InvalidateCache(
        PKIX_PL_Object *object,
        void *plContext);



















PKIX_Error *
PKIX_PL_Object_IncRef(
        PKIX_PL_Object *object,
        void *plContext);

























PKIX_Error *
PKIX_PL_Object_DecRef(
        PKIX_PL_Object *object,
        void *plContext);






























PKIX_Error *
PKIX_PL_Object_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext);





























PKIX_Error *
PKIX_PL_Object_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pValue,
        void *plContext);



























PKIX_Error *
PKIX_PL_Object_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext);
































PKIX_Error *
PKIX_PL_Object_Compare(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext);





























PKIX_Error *
PKIX_PL_Object_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext);





















PKIX_Error *
PKIX_PL_Object_GetType(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pType,
        void *plContext);




















PKIX_Error *
PKIX_PL_Object_Lock(
        PKIX_PL_Object *object,
        void *plContext);




















PKIX_Error *
PKIX_PL_Object_Unlock(
        PKIX_PL_Object *object,
        void *plContext);




















PKIX_Error *
PKIX_PL_Mutex_Create(
        PKIX_PL_Mutex **pNewLock,
        void *plContext);




















PKIX_Error *
PKIX_PL_Mutex_Lock(
        PKIX_PL_Mutex *lock,
        void *plContext);



















PKIX_Error *
PKIX_PL_Mutex_Unlock(
        PKIX_PL_Mutex *lock,
        void *plContext);




















PKIX_Error *
PKIX_PL_MonitorLock_Create(
        PKIX_PL_MonitorLock **pNewLock,
        void *plContext);




















PKIX_Error *
PKIX_PL_MonitorLock_Enter(
        PKIX_PL_MonitorLock *lock,
        void *plContext);



















PKIX_Error *
PKIX_PL_MonitorLock_Exit(
        PKIX_PL_MonitorLock *lock,
        void *plContext);






































PKIX_Error *
PKIX_PL_String_Create(
        PKIX_UInt32 fmtIndicator,
        const void *stringRep,
        PKIX_UInt32 stringLen,
        PKIX_PL_String **pString,
        void *plContext);


























PKIX_Error *
PKIX_PL_Sprintf(
        PKIX_PL_String **pOut,
        void *plContext,
        const PKIX_PL_String *fmt, ...);



























PKIX_Error *
PKIX_PL_GetString(
        PKIX_UInt32 stringID,
        char *defaultString,
        PKIX_PL_String **pString,
        void *plContext);































PKIX_Error *
PKIX_PL_String_GetEncoded(
        PKIX_PL_String *string,
        PKIX_UInt32 fmtIndicator,
        void **pStringRep,
        PKIX_UInt32 *pLength,
        void *plContext);





































PKIX_Error *
PKIX_PL_HashTable_Create(
        PKIX_UInt32 numBuckets,
        PKIX_UInt32 maxEntriesPerBucket,
        PKIX_PL_HashTable **pResult,
        void *plContext);




























PKIX_Error *
PKIX_PL_HashTable_Add(
        PKIX_PL_HashTable *ht,
        PKIX_PL_Object *key,
        PKIX_PL_Object *value,
        void *plContext);



























PKIX_Error *
PKIX_PL_HashTable_Remove(
        PKIX_PL_HashTable *ht,
        PKIX_PL_Object *key,
        void *plContext);


























PKIX_Error *
PKIX_PL_HashTable_Lookup(
        PKIX_PL_HashTable *ht,
        PKIX_PL_Object *key,
        PKIX_PL_Object **pResult,
        void *plContext);























PKIX_Error *
PKIX_PL_ByteArray_Create(
        void *array,
        PKIX_UInt32 length,
        PKIX_PL_ByteArray **pByteArray,
        void *plContext);
























PKIX_Error *
PKIX_PL_ByteArray_GetPointer(
        PKIX_PL_ByteArray *byteArray,
        void **pArray,
        void *plContext);





















PKIX_Error *
PKIX_PL_ByteArray_GetLength(
        PKIX_PL_ByteArray *byteArray,
        PKIX_UInt32 *pLength,
        void *plContext);





















PKIX_Error *
PKIX_PL_OID_Create(
        SECOidTag idtag,
        PKIX_PL_OID **pOID,
        void *plContext);





















PKIX_Error *
PKIX_PL_OID_CreateBySECItem(
        SECItem *derOid,
        PKIX_PL_OID **pOID,
        void *plContext);










































PKIX_Error *
PKIX_PL_BigInt_Create(
        PKIX_PL_String *stringRep,
        PKIX_PL_BigInt **pBigInt,
        void *plContext);

#ifdef __cplusplus
}
#endif












int
PKIX_PL_GetPLErrorCode();

#endif
