











#ifndef UHASH_H
#define UHASH_H

#include "unicode/utypes.h"
#include "cmemory.h"
#include "uelement.h"



























































U_CDECL_BEGIN






typedef UElement UHashTok;




struct UHashElement {
    
    int32_t  hashcode;
    UHashTok value;
    UHashTok key;
};
typedef struct UHashElement UHashElement;






typedef int32_t U_CALLCONV UHashFunction(const UHashTok key);




typedef UElementsAreEqual UKeyComparator;




typedef UElementsAreEqual UValueComparator;







enum UHashResizePolicy {
    U_GROW,            
    U_GROW_AND_SHRINK, 
    U_FIXED            
};





struct UHashtable {

    

    UHashElement *elements;

    

    UHashFunction *keyHasher;      

    UKeyComparator *keyComparator; 

    UValueComparator *valueComparator; 

    UObjectDeleter *keyDeleter;    

    UObjectDeleter *valueDeleter;  


    
  
    int32_t     count;      


    int32_t     length;     


    
    
    int32_t     highWaterMark;  
    int32_t     lowWaterMark;   
    float       highWaterRatio; 
    float       lowWaterRatio;  
    
    int8_t      primeIndex;     

    UBool       allocated; 
};
typedef struct UHashtable UHashtable;

U_CDECL_END















U_CAPI UHashtable* U_EXPORT2 
uhash_open(UHashFunction *keyHash,
           UKeyComparator *keyComp,
           UValueComparator *valueComp,
           UErrorCode *status);












U_CAPI UHashtable* U_EXPORT2 
uhash_openSize(UHashFunction *keyHash,
               UKeyComparator *keyComp,
               UValueComparator *valueComp,
               int32_t size,
               UErrorCode *status);











U_CAPI UHashtable* U_EXPORT2 
uhash_init(UHashtable *hash,
           UHashFunction *keyHash,
           UKeyComparator *keyComp,
           UValueComparator *valueComp,
           UErrorCode *status);





U_CAPI void U_EXPORT2 
uhash_close(UHashtable *hash);









U_CAPI UHashFunction *U_EXPORT2 
uhash_setKeyHasher(UHashtable *hash, UHashFunction *fn);








U_CAPI UKeyComparator *U_EXPORT2 
uhash_setKeyComparator(UHashtable *hash, UKeyComparator *fn);








U_CAPI UValueComparator *U_EXPORT2 
uhash_setValueComparator(UHashtable *hash, UValueComparator *fn);











U_CAPI UObjectDeleter *U_EXPORT2 
uhash_setKeyDeleter(UHashtable *hash, UObjectDeleter *fn);











U_CAPI UObjectDeleter *U_EXPORT2 
uhash_setValueDeleter(UHashtable *hash, UObjectDeleter *fn);








U_CAPI void U_EXPORT2 
uhash_setResizePolicy(UHashtable *hash, enum UHashResizePolicy policy);






U_CAPI int32_t U_EXPORT2 
uhash_count(const UHashtable *hash);














U_CAPI void* U_EXPORT2 
uhash_put(UHashtable *hash,
          void *key,
          void *value,
          UErrorCode *status);













U_CAPI void* U_EXPORT2 
uhash_iput(UHashtable *hash,
           int32_t key,
           void* value,
           UErrorCode *status);













U_CAPI int32_t U_EXPORT2 
uhash_puti(UHashtable *hash,
           void* key,
           int32_t value,
           UErrorCode *status);













U_CAPI int32_t U_EXPORT2 
uhash_iputi(UHashtable *hash,
           int32_t key,
           int32_t value,
           UErrorCode *status);








U_CAPI void* U_EXPORT2 
uhash_get(const UHashtable *hash, 
          const void *key);








U_CAPI void* U_EXPORT2 
uhash_iget(const UHashtable *hash,
           int32_t key);








U_CAPI int32_t U_EXPORT2 
uhash_geti(const UHashtable *hash,
           const void* key);







U_CAPI int32_t U_EXPORT2 
uhash_igeti(const UHashtable *hash,
           int32_t key);







U_CAPI void* U_EXPORT2 
uhash_remove(UHashtable *hash,
             const void *key);







U_CAPI void* U_EXPORT2 
uhash_iremove(UHashtable *hash,
              int32_t key);







U_CAPI int32_t U_EXPORT2 
uhash_removei(UHashtable *hash,
              const void* key);







U_CAPI int32_t U_EXPORT2 
uhash_iremovei(UHashtable *hash,
               int32_t key);





U_CAPI void U_EXPORT2 
uhash_removeAll(UHashtable *hash);












U_CAPI const UHashElement* U_EXPORT2 
uhash_find(const UHashtable *hash, const void* key);














U_CAPI const UHashElement* U_EXPORT2 
uhash_nextElement(const UHashtable *hash,
                  int32_t *pos);












U_CAPI void* U_EXPORT2 
uhash_removeElement(UHashtable *hash, const UHashElement* e);
































U_CAPI int32_t U_EXPORT2 
uhash_hashUChars(const UHashTok key);








U_CAPI int32_t U_EXPORT2 
uhash_hashChars(const UHashTok key);








U_CAPI int32_t U_EXPORT2
uhash_hashIChars(const UHashTok key);








U_CAPI UBool U_EXPORT2 
uhash_compareUChars(const UHashTok key1, const UHashTok key2);








U_CAPI UBool U_EXPORT2 
uhash_compareChars(const UHashTok key1, const UHashTok key2);








U_CAPI UBool U_EXPORT2 
uhash_compareIChars(const UHashTok key1, const UHashTok key2);










U_CAPI int32_t U_EXPORT2 
uhash_hashUnicodeString(const UElement key);







U_CAPI int32_t U_EXPORT2 
uhash_hashCaselessUnicodeString(const UElement key);










U_CAPI int32_t U_EXPORT2 
uhash_hashLong(const UHashTok key);







U_CAPI UBool U_EXPORT2 
uhash_compareLong(const UHashTok key1, const UHashTok key2);









U_CAPI void U_EXPORT2 
uhash_deleteHashtable(void *obj);









U_CAPI UBool U_EXPORT2 
uhash_equals(const UHashtable* hash1, const UHashtable* hash2);

#endif
