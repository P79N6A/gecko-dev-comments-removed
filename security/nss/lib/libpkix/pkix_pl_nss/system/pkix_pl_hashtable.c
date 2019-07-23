










































#include "pkix_pl_hashtable.h"



struct PKIX_PL_HashTableStruct {
        pkix_pl_PrimHashTable *primHash;
        PKIX_PL_Mutex *tableLock;
        PKIX_UInt32 maxEntriesPerBucket;
};



#define PKIX_MUTEX_UNLOCK(mutex) \
    do { \
        if (mutex && lockedMutex == (PKIX_PL_Mutex *)(mutex)) { \
            pkixTempResult = \
                PKIX_PL_Mutex_Unlock((mutex), plContext); \
            PORT_Assert(pkixTempResult == NULL); \
            if (pkixTempResult) { \
                PKIX_DoAddError(&stdVars, pkixTempResult, plContext); \
                pkixTempResult = NULL; \
            } \
            lockedMutex = NULL; \
        } else { \
            PORT_Assert(lockedMutex == NULL); \
        }\
    } while (0)


#define PKIX_MUTEX_LOCK(mutex) \
    do { \
        if (mutex){ \
            PORT_Assert(lockedMutex == NULL); \
            PKIX_CHECK(PKIX_PL_Mutex_Lock((mutex), plContext), \
                       PKIX_MUTEXLOCKFAILED); \
            lockedMutex = (mutex); \
        } \
    } while (0)





static PKIX_Error *
pkix_pl_HashTable_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_HashTable *ht = NULL;
        pkix_pl_HT_Elem *item = NULL;
        PKIX_UInt32 i;

        PKIX_ENTER(HASHTABLE, "pkix_pl_HashTable_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType(object, PKIX_HASHTABLE_TYPE, plContext),
                    PKIX_OBJECTNOTHASHTABLE);

        ht = (PKIX_PL_HashTable*) object;

        
        for (i = 0; i < ht->primHash->size; i++) {
                for (item = ht->primHash->buckets[i];
                    item != NULL;
                    item = item->next) {
                        PKIX_DECREF(item->key);
                        PKIX_DECREF(item->value);
                }
        }

        PKIX_CHECK(pkix_pl_PrimHashTable_Destroy(ht->primHash, plContext),
                    PKIX_PRIMHASHTABLEDESTROYFAILED);

        PKIX_DECREF(ht->tableLock);

cleanup:

        PKIX_RETURN(HASHTABLE);
}












PKIX_Error *
pkix_pl_HashTable_RegisterSelf(
        void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(HASHTABLE, "pkix_pl_HashTable_RegisterSelf");

        entry.description = "HashTable";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_HashTable);
        entry.destructor = pkix_pl_HashTable_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_HASHTABLE_TYPE] = entry;

        PKIX_RETURN(HASHTABLE);
}






PKIX_Error *
PKIX_PL_HashTable_Create(
        PKIX_UInt32 numBuckets,
        PKIX_UInt32 maxEntriesPerBucket,
        PKIX_PL_HashTable **pResult,
        void *plContext)
{
        PKIX_PL_HashTable *hashTable = NULL;

        PKIX_ENTER(HASHTABLE, "PKIX_PL_HashTable_Create");
        PKIX_NULLCHECK_ONE(pResult);

        if (numBuckets == 0) {
                PKIX_ERROR(PKIX_NUMBUCKETSEQUALSZERO);
        }

        
        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_HASHTABLE_TYPE,
                sizeof (PKIX_PL_HashTable),
                (PKIX_PL_Object **)&hashTable,
                plContext),
                PKIX_COULDNOTCREATEHASHTABLEOBJECT);

        
        PKIX_CHECK(pkix_pl_PrimHashTable_Create
                    (numBuckets, &hashTable->primHash, plContext),
                    PKIX_PRIMHASHTABLECREATEFAILED);

        
        PKIX_CHECK(PKIX_PL_Mutex_Create(&hashTable->tableLock, plContext),
                    PKIX_ERRORCREATINGTABLELOCK);

        hashTable->maxEntriesPerBucket = maxEntriesPerBucket;

        *pResult = hashTable;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(hashTable);
        }

        PKIX_RETURN(HASHTABLE);
}




PKIX_Error *
PKIX_PL_HashTable_Add(
        PKIX_PL_HashTable *ht,
        PKIX_PL_Object *key,
        PKIX_PL_Object *value,
        void *plContext)
{
        PKIX_PL_Mutex  *lockedMutex = NULL;
        PKIX_PL_Object *deletedKey = NULL;
        PKIX_PL_Object *deletedValue = NULL;
        PKIX_UInt32 hashCode;
        PKIX_PL_EqualsCallback keyComp;
        PKIX_UInt32 bucketSize = 0;

        PKIX_ENTER(HASHTABLE, "PKIX_PL_HashTable_Add");

#if !defined(PKIX_OBJECT_LEAK_TEST)
        PKIX_NULLCHECK_THREE(ht, key, value);
#else
        PKIX_NULLCHECK_TWO(key, value);

        if (ht == NULL) {
            PKIX_RETURN(HASHTABLE);
        }
#endif
        

        PKIX_CHECK(PKIX_PL_Object_Hashcode(key, &hashCode, plContext),
                    PKIX_OBJECTHASHCODEFAILED);

        PKIX_CHECK(pkix_pl_Object_RetrieveEqualsCallback
                    (key, &keyComp, plContext),
                    PKIX_OBJECTRETRIEVEEQUALSCALLBACKFAILED);

        PKIX_MUTEX_LOCK(ht->tableLock);

        PKIX_CHECK(pkix_pl_PrimHashTable_GetBucketSize
                (ht->primHash,
                hashCode,
                &bucketSize,
                plContext),
                PKIX_PRIMHASHTABLEGETBUCKETSIZEFAILED);

        if (ht->maxEntriesPerBucket != 0 &&
            bucketSize >= ht->maxEntriesPerBucket) {
                
                PKIX_CHECK(pkix_pl_PrimHashTable_RemoveFIFO
                        (ht->primHash,
                        hashCode,
                        (void **) &deletedKey,
                        (void **) &deletedValue,
                        plContext),
                        PKIX_PRIMHASHTABLEGETBUCKETSIZEFAILED);
                PKIX_DECREF(deletedKey);
                PKIX_DECREF(deletedValue);
        }

        PKIX_CHECK(pkix_pl_PrimHashTable_Add
                (ht->primHash,
                (void *)key,
                (void *)value,
                hashCode,
                keyComp,
                plContext),
                PKIX_PRIMHASHTABLEADDFAILED);

        PKIX_INCREF(key);
        PKIX_INCREF(value);
        PKIX_MUTEX_UNLOCK(ht->tableLock);

        




cleanup:

        PKIX_MUTEX_UNLOCK(ht->tableLock);

        PKIX_RETURN(HASHTABLE);
}




PKIX_Error *
PKIX_PL_HashTable_Remove(
        PKIX_PL_HashTable *ht,
        PKIX_PL_Object *key,
        void *plContext)
{
        PKIX_PL_Mutex  *lockedMutex = NULL;
        PKIX_PL_Object *origKey = NULL;
        PKIX_PL_Object *value = NULL;
        PKIX_UInt32 hashCode;
        PKIX_PL_EqualsCallback keyComp;

        PKIX_ENTER(HASHTABLE, "PKIX_PL_HashTable_Remove");

#if !defined(PKIX_OBJECT_LEAK_TEST)
        PKIX_NULLCHECK_TWO(ht, key);
#else
        PKIX_NULLCHECK_ONE(key);

        if (ht == NULL) {
            PKIX_RETURN(HASHTABLE);
        }
#endif

        PKIX_CHECK(PKIX_PL_Object_Hashcode(key, &hashCode, plContext),
                    PKIX_OBJECTHASHCODEFAILED);

        PKIX_CHECK(pkix_pl_Object_RetrieveEqualsCallback
                    (key, &keyComp, plContext),
                    PKIX_OBJECTRETRIEVEEQUALSCALLBACKFAILED);

        PKIX_MUTEX_LOCK(ht->tableLock);

        
        PKIX_CHECK(pkix_pl_PrimHashTable_Remove
                (ht->primHash,
                (void *)key,
                hashCode,
                keyComp,
                (void **)&origKey,
                (void **)&value,
                plContext),
                PKIX_PRIMHASHTABLEREMOVEFAILED);

        PKIX_MUTEX_UNLOCK(ht->tableLock);

        PKIX_DECREF(origKey);
        PKIX_DECREF(value);

        




cleanup:

        PKIX_MUTEX_UNLOCK(ht->tableLock);

        PKIX_RETURN(HASHTABLE);
}




PKIX_Error *
PKIX_PL_HashTable_Lookup(
        PKIX_PL_HashTable *ht,
        PKIX_PL_Object *key,
        PKIX_PL_Object **pResult,
        void *plContext)
{
        PKIX_PL_Mutex *lockedMutex = NULL;
        PKIX_UInt32 hashCode;
        PKIX_PL_EqualsCallback keyComp;
        PKIX_PL_Object *result = NULL;

        PKIX_ENTER(HASHTABLE, "PKIX_PL_HashTable_Lookup");

#if !defined(PKIX_OBJECT_LEAK_TEST)
        PKIX_NULLCHECK_THREE(ht, key, pResult);
#else
        PKIX_NULLCHECK_TWO(key, pResult);

        if (ht == NULL) {
            PKIX_RETURN(HASHTABLE);
        }
#endif

        PKIX_CHECK(PKIX_PL_Object_Hashcode(key, &hashCode, plContext),
                    PKIX_OBJECTHASHCODEFAILED);

        PKIX_CHECK(pkix_pl_Object_RetrieveEqualsCallback
                    (key, &keyComp, plContext),
                    PKIX_OBJECTRETRIEVEEQUALSCALLBACKFAILED);

        PKIX_MUTEX_LOCK(ht->tableLock);

        
        PKIX_CHECK(pkix_pl_PrimHashTable_Lookup
                (ht->primHash,
                (void *)key,
                hashCode,
                keyComp,
                (void **)&result,
                plContext),
                PKIX_PRIMHASHTABLELOOKUPFAILED);

        PKIX_INCREF(result);
        PKIX_MUTEX_UNLOCK(ht->tableLock);

        *pResult = result;

cleanup:
        
        PKIX_MUTEX_UNLOCK(ht->tableLock);

        PKIX_RETURN(HASHTABLE);
}
