









#include "pkix_pl_primhash.h"




























static PKIX_Error *
pkix_pl_KeyComparator_Default(
        PKIX_UInt32 *firstKey,
        PKIX_UInt32 *secondKey,
        PKIX_Boolean *pResult,
        void *plContext)
{
        
        PKIX_UInt32 firstInt, secondInt;

        PKIX_ENTER(HASHTABLE, "pkix_pl_KeyComparator_Default");
        PKIX_NULLCHECK_THREE(firstKey, secondKey, pResult);

        firstInt = *firstKey;
        secondInt = *secondKey;

        *pResult = (firstInt == secondInt)?PKIX_TRUE:PKIX_FALSE;

        PKIX_RETURN(HASHTABLE);
}






















PKIX_Error *
pkix_pl_PrimHashTable_Create(
        PKIX_UInt32 numBuckets,
        pkix_pl_PrimHashTable **pResult,
        void *plContext)
{
        pkix_pl_PrimHashTable *primHashTable = NULL;
        PKIX_UInt32 i;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_Create");
        PKIX_NULLCHECK_ONE(pResult);

        if (numBuckets == 0) {
                PKIX_ERROR(PKIX_NUMBUCKETSEQUALSZERO);
        }

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (sizeof (pkix_pl_PrimHashTable),
                    (void **)&primHashTable,
                    plContext),
                    PKIX_MALLOCFAILED);

        primHashTable->size = numBuckets;

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (numBuckets * sizeof (pkix_pl_HT_Elem*),
                    (void **)&primHashTable->buckets,
                    plContext),
                    PKIX_MALLOCFAILED);

        for (i = 0; i < numBuckets; i++) {
                primHashTable->buckets[i] = NULL;
        }

        *pResult = primHashTable;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_FREE(primHashTable);
        }

        PKIX_RETURN(HASHTABLE);
}


































PKIX_Error *
pkix_pl_PrimHashTable_Add(
        pkix_pl_PrimHashTable *ht,
        void *key,
        void *value,
        PKIX_UInt32 hashCode,
        PKIX_PL_EqualsCallback keyComp,
        void *plContext)
{
        pkix_pl_HT_Elem **elemPtr = NULL;
        pkix_pl_HT_Elem *element = NULL;
        PKIX_Boolean compResult = PKIX_FALSE;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_Add");
        PKIX_NULLCHECK_THREE(ht, key, value);

        for (elemPtr = &((ht->buckets)[hashCode%ht->size]), element = *elemPtr;
            element != NULL; elemPtr = &(element->next), element = *elemPtr) {

                if (element->hashCode != hashCode){
                        
                        continue;
                }

                if (keyComp == NULL){
                        PKIX_CHECK(pkix_pl_KeyComparator_Default
                                ((PKIX_UInt32 *)key,
                                (PKIX_UInt32 *)(element->key),
                                &compResult,
                                plContext),
                                PKIX_COULDNOTTESTWHETHERKEYSEQUAL);
                } else {
                        PKIX_CHECK(keyComp
                                ((PKIX_PL_Object *)key,
                                (PKIX_PL_Object *)(element->key),
                                &compResult,
                                plContext),
                                PKIX_COULDNOTTESTWHETHERKEYSEQUAL);
                }

                if ((element->hashCode == hashCode) &&
                    (compResult == PKIX_TRUE)){
                        
                    PKIX_ERROR(PKIX_ATTEMPTTOADDDUPLICATEKEY);
                }
        }

        
        if (element != NULL) {
                PKIX_ERROR(PKIX_ERRORTRAVERSINGBUCKET);
        }

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (sizeof (pkix_pl_HT_Elem), (void **)elemPtr, plContext),
                    PKIX_MALLOCFAILED);

        element = *elemPtr;

        element->key = key;
        element->value = value;
        element->hashCode = hashCode;
        element->next = NULL;

cleanup:

        PKIX_RETURN(HASHTABLE);
}




































PKIX_Error *
pkix_pl_PrimHashTable_Remove(
        pkix_pl_PrimHashTable *ht,
        void *key,
        PKIX_UInt32 hashCode,
        PKIX_PL_EqualsCallback keyComp,
        void **pKey,
        void **pValue,
        void *plContext)
{
        pkix_pl_HT_Elem *element = NULL;
        pkix_pl_HT_Elem *prior = NULL;
        PKIX_Boolean compResult;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_Remove");
        PKIX_NULLCHECK_FOUR(ht, key, pKey, pValue);

        *pKey = NULL;
        *pValue = NULL;

        for (element = ht->buckets[hashCode%ht->size], prior = element;
            (element != NULL);
            prior = element, element = element->next) {

                if (element->hashCode != hashCode){
                        
                        continue;
                }

                if (keyComp == NULL){
                        PKIX_CHECK(pkix_pl_KeyComparator_Default
                                ((PKIX_UInt32 *)key,
                                (PKIX_UInt32 *)(element->key),
                                &compResult,
                                plContext),
                                PKIX_COULDNOTTESTWHETHERKEYSEQUAL);
                } else {
                        PKIX_CHECK(keyComp
                                ((PKIX_PL_Object *)key,
                                (PKIX_PL_Object *)(element->key),
                                &compResult,
                                plContext),
                                PKIX_COULDNOTTESTWHETHERKEYSEQUAL);
                }

                if ((element->hashCode == hashCode) &&
                    (compResult == PKIX_TRUE)){
                        if (element != prior) {
                                prior->next = element->next;
                        } else {
                                ht->buckets[hashCode%ht->size] = element->next;
                        }
                        *pKey = element->key;
                        *pValue = element->value;
                        element->key = NULL;
                        element->value = NULL;
                        element->next = NULL;
                        PKIX_FREE(element);
                        goto cleanup;
                }
        }

cleanup:

        PKIX_RETURN(HASHTABLE);
}


































PKIX_Error *
pkix_pl_PrimHashTable_Lookup(
        pkix_pl_PrimHashTable *ht,
        void *key,
        PKIX_UInt32 hashCode,
        PKIX_PL_EqualsCallback keyComp,
        void **pResult,
        void *plContext)
{
        pkix_pl_HT_Elem *element = NULL;
        PKIX_Boolean compResult = PKIX_FALSE;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_Lookup");
        PKIX_NULLCHECK_THREE(ht, key, pResult);

        *pResult = NULL;

        for (element = (ht->buckets)[hashCode%ht->size];
            (element != NULL) && (*pResult == NULL);
            element = element->next) {

                if (element->hashCode != hashCode){
                        
                        continue;
                }

                if (keyComp == NULL){
                        PKIX_CHECK(pkix_pl_KeyComparator_Default
                                ((PKIX_UInt32 *)key,
                                (PKIX_UInt32 *)(element->key),
                                &compResult,
                                plContext),
                                PKIX_COULDNOTTESTWHETHERKEYSEQUAL);
                } else {
                       pkixErrorResult =
                           keyComp((PKIX_PL_Object *)key,
                                   (PKIX_PL_Object *)(element->key),
                                   &compResult,
                                   plContext);
                       if (pkixErrorResult) {
                           pkixErrorClass = PKIX_FATAL_ERROR;
                           pkixErrorCode = PKIX_COULDNOTTESTWHETHERKEYSEQUAL;
                           goto cleanup;
                       }
                }

                if ((element->hashCode == hashCode) &&
                    (compResult == PKIX_TRUE)){
                        *pResult = element->value;
                        goto cleanup;
                }
        }

        
        *pResult = NULL;

cleanup:

        PKIX_RETURN(HASHTABLE);
}



















PKIX_Error *
pkix_pl_PrimHashTable_Destroy(
        pkix_pl_PrimHashTable *ht,
        void *plContext)
{
        pkix_pl_HT_Elem *element = NULL;
        pkix_pl_HT_Elem *temp = NULL;
        PKIX_UInt32 i;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_Destroy");
        PKIX_NULLCHECK_ONE(ht);

        
        for (i = 0; i < ht->size; i++) {
                for (element = ht->buckets[i];
                    element != NULL;
                    element = temp) {
                        temp = element->next;
                        element->value = NULL;
                        element->key = NULL;
                        element->hashCode = 0;
                        element->next = NULL;
                        PKIX_FREE(element);
                }
        }

        
        PKIX_FREE(ht->buckets);
        ht->size = 0;

        
        PKIX_FREE(ht);

        PKIX_RETURN(HASHTABLE);
}


























PKIX_Error *
pkix_pl_PrimHashTable_GetBucketSize(
        pkix_pl_PrimHashTable *ht,
        PKIX_UInt32 hashCode,
        PKIX_UInt32 *pBucketSize,
        void *plContext)
{
        pkix_pl_HT_Elem **elemPtr = NULL;
        pkix_pl_HT_Elem *element = NULL;
        PKIX_UInt32 bucketSize = 0;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_GetBucketSize");
        PKIX_NULLCHECK_TWO(ht, pBucketSize);

        for (elemPtr = &((ht->buckets)[hashCode%ht->size]), element = *elemPtr;
            element != NULL; elemPtr = &(element->next), element = *elemPtr) {
                bucketSize++;
	}

        *pBucketSize = bucketSize;

        PKIX_RETURN(HASHTABLE);
}




























PKIX_Error *
pkix_pl_PrimHashTable_RemoveFIFO(
        pkix_pl_PrimHashTable *ht,
        PKIX_UInt32 hashCode,
        void **pKey,
        void **pValue,
        void *plContext)
{
        pkix_pl_HT_Elem *element = NULL;

        PKIX_ENTER(HASHTABLE, "pkix_pl_PrimHashTable_Remove");
        PKIX_NULLCHECK_THREE(ht, pKey, pValue);

        element = (ht->buckets)[hashCode%ht->size];

        if (element != NULL) {

                *pKey = element->key;
                *pValue = element->value;
                ht->buckets[hashCode%ht->size] = element->next;
                element->key = NULL;
                element->value = NULL;
                element->next = NULL;
                PKIX_FREE(element);
        }

        PKIX_RETURN(HASHTABLE);
}
