










































#include "pkix_pl_object.h"

#ifdef PKIX_USER_OBJECT_TYPE








static pkix_pl_HT_Elem*
pkix_Raw_ClassTable_Buckets[] = {
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};





static pkix_pl_PrimHashTable pkix_Raw_ClassTable = {
        (void *)pkix_Raw_ClassTable_Buckets, 
        20 
};
static pkix_pl_PrimHashTable * classTable = &pkix_Raw_ClassTable;
#endif 























static PKIX_Error *
pkix_pl_Object_GetHeader(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pObjectHeader,
        void *plContext)
{
        PKIX_PL_Object *header = NULL;
        PKIX_UInt32 objType;

        PKIX_ENTER(OBJECT, "pkix_pl_Object_GetHeader");
        PKIX_NULLCHECK_TWO(object, pObjectHeader);

        PKIX_OBJECT_DEBUG("\tShifting object pointer).\n");

        
        header = (PKIX_PL_Object *)((char *)object - sizeof(PKIX_PL_Object));

        objType = header->type;

        if (objType >= PKIX_NUMTYPES) { 
#ifdef PKIX_USER_OBJECT_TYPE
                pkix_ClassTable_Entry *ctEntry = NULL;

                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);

                PKIX_CHECK(pkix_pl_PrimHashTable_Lookup
                            (classTable,
                            (void *)&objType,
                            objType,
                            NULL,
                            (void **)&ctEntry,
                            plContext),
                            PKIX_ERRORGETTINGCLASSTABLEENTRY);

                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);

                if (ctEntry == NULL) {
                        PKIX_ERROR_FATAL(PKIX_UNKNOWNOBJECTTYPE);
                }
#else
                PORT_Assert(objType < PKIX_NUMTYPES);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        }

#ifdef PKIX_OBJECT_LEAK_TEST
        PORT_Assert(header && header->magicHeader == PKIX_MAGIC_HEADER);
#endif 

        if ((header == NULL)||
            (header->magicHeader != PKIX_MAGIC_HEADER)) {
                PKIX_ERROR_ALLOC_ERROR();
        }

        *pObjectHeader = header;

cleanup:

        PKIX_RETURN(OBJECT);
}




















static PKIX_Error *
pkix_pl_Object_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;

        PKIX_ENTER(OBJECT, "pkix_pl_Object_Destroy");
        PKIX_NULLCHECK_ONE(object);

#ifdef PKIX_OBJECT_LEAK_TEST
        PKIX_CHECK_FATAL(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);
#else
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);
#endif 

        
        if (objectHeader->references != 0) {
                PKIX_ERROR_FATAL(PKIX_OBJECTSTILLREFERENCED);
        }

        PKIX_DECREF(objectHeader->stringRep);

        
        PKIX_OBJECT_DEBUG("\tCalling PR_DestroyLock).\n");
        PR_DestroyLock(objectHeader->lock);
        objectHeader->lock = NULL;
        object = NULL;

        objectHeader->magicHeader = PKIX_MAGIC_HEADER_DESTROYED;

#ifdef PKIX_OBJECT_LEAK_TEST
        memset(objectHeader, 0xbf, systemClasses[PKIX_OBJECT_TYPE].typeObjectSize);
#endif

        PKIX_FREE(objectHeader);

cleanup:
#ifdef PKIX_OBJECT_LEAK_TEST
fatal:
#endif

        PKIX_RETURN(OBJECT);
}


























static PKIX_Error *
pkix_pl_Object_Equals_Default(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_ENTER(OBJECT, "pkix_pl_Object_Equals_Default");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        *pResult = (firstObject == secondObject)?PKIX_TRUE:PKIX_FALSE;

        PKIX_RETURN(OBJECT);
}
























static PKIX_Error *
pkix_pl_Object_ToString_Default(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *descString = NULL;
        char *format = "%s@Address: %x";
        char *description = NULL;
        PKIX_UInt32 objType;

        PKIX_ENTER(OBJECT, "pkix_pl_Object_ToString_Default");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(PKIX_PL_Object_GetType(object, &objType, plContext),
                    PKIX_OBJECTGETTYPEFAILED);

        if (objType >= PKIX_NUMTYPES){
#ifdef PKIX_USER_OBJECT_TYPE
                pkix_ClassTable_Entry *ctEntry = NULL;

                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                        (classTable,
                        (void *)&objType,
                        objType,
                        NULL,
                        (void **)&ctEntry,
                        plContext);
                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);
                if (pkixErrorResult){
                        PKIX_ERROR_FATAL(PKIX_ERRORGETTINGCLASSTABLEENTRY);
                }

                if (ctEntry == NULL){
                        PKIX_ERROR_FATAL(PKIX_UNDEFINEDCLASSTABLEENTRY);
                } else {
                        description = ctEntry->description;
                        if (description == NULL) {
                            description = "User Type Object";
                        }
                }
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        } else {
                description = systemClasses[objType].description;
        }
        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    (void *)format,
                    0,
                    &formatString,
                    plContext),
                    PKIX_STRINGCREATEFAILED);

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    (void *)description,
                    0,
                    &descString,
                    plContext),
                    PKIX_STRINGCREATEFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                    (pString,
                    plContext,
                    formatString,
                    descString,
                    object),
                    PKIX_SPRINTFFAILED);

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(descString);

        PKIX_RETURN(OBJECT);
}
























static PKIX_Error *
pkix_pl_Object_Hashcode_Default(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pValue,
        void *plContext)
{
        PKIX_ENTER(OBJECT, "pkix_pl_Object_Hashcode_Default");
        PKIX_NULLCHECK_TWO(object, pValue);

        *pValue = (PKIX_UInt32)object;

        PKIX_RETURN(OBJECT);
}


























PKIX_Error *
pkix_pl_Object_RetrieveEqualsCallback(
        PKIX_PL_Object *object,
        PKIX_PL_EqualsCallback *pEqualsCallback,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;
        PKIX_PL_EqualsCallback func = NULL;
        pkix_ClassTable_Entry entry;
        PKIX_UInt32 objType;

        PKIX_ENTER(OBJECT, "pkix_pl_Object_RetrieveEqualsCallback");
        PKIX_NULLCHECK_TWO(object, pEqualsCallback);

        PKIX_CHECK(pkix_pl_Object_GetHeader
                    (object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        objType = objectHeader->type;

        if (objType >= PKIX_NUMTYPES){
#ifdef PKIX_USER_OBJECT_TYPE
                pkix_ClassTable_Entry *ctEntry = NULL;

                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                        (classTable,
                        (void *)&objType,
                        objType,
                        NULL,
                        (void **)&ctEntry,
                        plContext);
                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);
                if (pkixErrorResult){
                        PKIX_ERROR(PKIX_ERRORGETTINGCLASSTABLEENTRY);
                }

                if ((ctEntry == NULL) || (ctEntry->equalsFunction == NULL)) {
                        PKIX_ERROR(PKIX_UNDEFINEDEQUALSCALLBACK);
                } else {
                        *pEqualsCallback = ctEntry->equalsFunction;
                }
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        } else {
                entry = systemClasses[objType];
                func = entry.equalsFunction;
                if (func == NULL){
                        func = pkix_pl_Object_Equals_Default;
                }
                *pEqualsCallback = func;
        }

cleanup:

        PKIX_RETURN(OBJECT);
}
















PKIX_Error *
pkix_pl_Object_RegisterSelf(void *plContext)
{
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(ERROR, "pkix_pl_Object_RegisterSelf");

        entry.description = "Object";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_Object);
        entry.destructor = NULL;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_OBJECT_TYPE] = entry;

        PKIX_RETURN(ERROR);
}






PKIX_Error *
PKIX_PL_Object_Alloc(
        PKIX_TYPENUM objType,
        PKIX_UInt32 size,
        PKIX_PL_Object **pObject,
        void *plContext)
{
        PKIX_PL_Object *object = NULL;
        pkix_ClassTable_Entry *ctEntry = NULL;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Alloc");
        PKIX_NULLCHECK_ONE(pObject);

        




        if (objType >= PKIX_NUMTYPES) { 
#ifdef PKIX_USER_OBJECT_TYPE
                PKIX_Boolean typeRegistered;
                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                        (classTable,
                        (void *)&objType,
                        objType,
                        NULL,
                        (void **)&ctEntry,
                        plContext);
                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);
                if (pkixErrorResult){
                        PKIX_ERROR_FATAL(PKIX_COULDNOTLOOKUPINHASHTABLE);
                }

                typeRegistered = (ctEntry != NULL);

                if (!typeRegistered) {
                        PKIX_ERROR_FATAL(PKIX_UNKNOWNTYPEARGUMENT);
                }
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        } else {
                ctEntry = &systemClasses[objType];
        }
        
        PORT_Assert(size == ctEntry->typeObjectSize);

        
#ifdef PKIX_OBJECT_LEAK_TEST       
        PKIX_CHECK(PKIX_PL_Calloc
                    (1,
                    ((PKIX_UInt32)sizeof (PKIX_PL_Object))+size,
                    (void **)&object,
                    plContext),
                    PKIX_MALLOCFAILED);
#else
        PKIX_CHECK(PKIX_PL_Malloc
                    (((PKIX_UInt32)sizeof (PKIX_PL_Object))+size,
                    (void **)&object,
                    plContext),
                    PKIX_MALLOCFAILED);
#endif 

        
        object->magicHeader = PKIX_MAGIC_HEADER;
        object->type = objType;
        object->references = 1; 
        object->stringRep = NULL;
        object->hashcode = 0;
        object->hashcodeCached = 0;

        
        
        PKIX_OBJECT_DEBUG("\tCalling PR_NewLock).\n");
        object->lock = PR_NewLock();
        if (object->lock == NULL) {
                PKIX_ERROR_ALLOC_ERROR();
        }

        PKIX_OBJECT_DEBUG("\tShifting object pointer).\n");


        
        *pObject = object + 1;
        object = NULL;

        
        PR_AtomicIncrement(&ctEntry->objCounter);

cleanup:

        PKIX_FREE(object);

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_IsTypeRegistered(
        PKIX_UInt32 objType,
        PKIX_Boolean *pBool,
        void *plContext)
{
#ifdef PKIX_USER_OBJECT_TYPE
        pkix_ClassTable_Entry *ctEntry = NULL;
#endif

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_IsTypeRegistered");
        PKIX_NULLCHECK_ONE(pBool);

        
        if (objType < PKIX_NUMTYPES) {
                *pBool = PKIX_TRUE;
                goto cleanup;
        }

#ifndef PKIX_USER_OBJECT_TYPE
        PORT_Assert (0);
        pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
        pkixErrorClass = PKIX_FATAL_ERROR;
#else
        PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
        PR_Lock(classTableLock);
        pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                (classTable,
                (void *)&objType,
                objType,
                NULL,
                (void **)&ctEntry,
                plContext);
        PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
        PR_Unlock(classTableLock);

        if (pkixErrorResult){
                PKIX_ERROR_FATAL(PKIX_COULDNOTLOOKUPINHASHTABLE);
        }

        *pBool = (ctEntry != NULL);
#endif 

cleanup:

        PKIX_RETURN(OBJECT);
}

#ifdef PKIX_USER_OBJECT_TYPE



PKIX_Error *
PKIX_PL_Object_RegisterType(
        PKIX_UInt32 objType,
        char *description,
        PKIX_PL_DestructorCallback destructor,
        PKIX_PL_EqualsCallback equalsFunction,
        PKIX_PL_HashcodeCallback hashcodeFunction,
        PKIX_PL_ToStringCallback toStringFunction,
        PKIX_PL_ComparatorCallback comparator,
        PKIX_PL_DuplicateCallback duplicateFunction,
        void *plContext)
{
        pkix_ClassTable_Entry *ctEntry = NULL;
        pkix_pl_Integer *key = NULL;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_RegisterType");

        




        if (objType < PKIX_NUMTYPES) { 
                PKIX_ERROR(PKIX_CANTREREGISTERSYSTEMTYPE);
        }

        PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
        PR_Lock(classTableLock);
        PKIX_CHECK(pkix_pl_PrimHashTable_Lookup
                    (classTable,
                    (void *)&objType,
                    objType,
                    NULL,
                    (void **)&ctEntry,
                    plContext),
                    PKIX_PRIMHASHTABLELOOKUPFAILED);

        
        if (ctEntry) {
                PKIX_ERROR(PKIX_TYPEALREADYREGISTERED);
        }

        PKIX_CHECK(PKIX_PL_Malloc
                    (((PKIX_UInt32)sizeof (pkix_ClassTable_Entry)),
                    (void **)&ctEntry,
                    plContext),
                    PKIX_MALLOCFAILED);

        

        if (description == NULL){
                description = "Object";
        }

        if (equalsFunction == NULL) {
                equalsFunction = pkix_pl_Object_Equals_Default;
        }

        if (toStringFunction == NULL) {
                toStringFunction = pkix_pl_Object_ToString_Default;
        }

        if (hashcodeFunction == NULL) {
                hashcodeFunction = pkix_pl_Object_Hashcode_Default;
        }

        ctEntry->destructor = destructor;
        ctEntry->equalsFunction = equalsFunction;
        ctEntry->toStringFunction = toStringFunction;
        ctEntry->hashcodeFunction = hashcodeFunction;
        ctEntry->comparator = comparator;
        ctEntry->duplicateFunction = duplicateFunction;
        ctEntry->description = description;

        PKIX_CHECK(PKIX_PL_Malloc
                    (((PKIX_UInt32)sizeof (pkix_pl_Integer)),
                    (void **)&key,
                    plContext),
                    PKIX_COULDNOTMALLOCNEWKEY);

        key->ht_int = objType;

        PKIX_CHECK(pkix_pl_PrimHashTable_Add
                    (classTable,
                    (void *)key,
                    (void *)ctEntry,
                    objType,
                    NULL,
                    plContext),
                    PKIX_PRIMHASHTABLEADDFAILED);

cleanup:
        PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
        PR_Unlock(classTableLock);

        PKIX_RETURN(OBJECT);
}
#endif 




PKIX_Error *
PKIX_PL_Object_IncRef(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;
        PKIX_PL_NssContext *context = NULL;
        PKIX_Int32 refCount = 0;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_IncRef");
        PKIX_NULLCHECK_ONE(object);

        if (plContext){
                



  
                context = (PKIX_PL_NssContext *) plContext;
                if (context->arena != NULL) {
                        goto cleanup;
                }
        }

        if (object == (PKIX_PL_Object*)PKIX_ALLOC_ERROR()) {
                goto cleanup;
        }

        
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        
        refCount = PR_AtomicIncrement(&objectHeader->references);

        if (refCount <= 1) {
                PKIX_THROW(FATAL, PKIX_OBJECTWITHNONPOSITIVEREFERENCES);
        }

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_DecRef(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_Int32 refCount = 0;
        PKIX_PL_Object *objectHeader = NULL;
        PKIX_PL_NssContext *context = NULL;
            
        PKIX_ENTER(OBJECT, "PKIX_PL_Object_DecRef");
        PKIX_NULLCHECK_ONE(object);

        if (plContext){
                



  
                context = (PKIX_PL_NssContext *) plContext;
                if (context->arena != NULL) {
                        goto cleanup;
                }
        }

        if (object == (PKIX_PL_Object*)PKIX_ALLOC_ERROR()) {
                goto cleanup;
        }

        
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        refCount = PR_AtomicDecrement(&objectHeader->references);

        if (refCount == 0) {
            PKIX_PL_DestructorCallback destructor = NULL;
            pkix_ClassTable_Entry *ctEntry = NULL;
            PKIX_UInt32 objType = objectHeader->type;
            
            
            if (objType >= PKIX_NUMTYPES){
#ifdef PKIX_USER_OBJECT_TYPE
                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                    (classTable,
                     (void *)&objType,
                     objType,
                     NULL,
                     (void **)&ctEntry,
                     plContext);
                PKIX_OBJECT_DEBUG
                    ("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);
                if (pkixErrorResult){
                    PKIX_ERROR_FATAL
                        (PKIX_ERRORINGETTINGDESTRUCTOR);
                }
                
                if (ctEntry != NULL){
                    destructor = ctEntry->destructor;
                }
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
            } else {
                ctEntry = &systemClasses[objType];
                destructor = ctEntry->destructor;
            }
            
            if (destructor != NULL){
                
                pkixErrorResult = destructor(object, plContext);
                if (pkixErrorResult) {
                    pkixErrorClass = PKIX_FATAL_ERROR;
                    PKIX_DoAddError(stdVarsPtr, pkixErrorResult, plContext);
                    pkixErrorResult = NULL;
                }
            }
            
            
            PR_AtomicDecrement(&ctEntry->objCounter);
            
            
            
            pkixErrorResult = pkix_pl_Object_Destroy(object, plContext);
            goto cleanup;
        }

        if (refCount < 0) {
            PKIX_ERROR_ALLOC_ERROR();
        }

cleanup:

        PKIX_RETURN(OBJECT);
}






PKIX_Error *
PKIX_PL_Object_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_Object *firstObjectHeader = NULL;
        PKIX_PL_Object *secondObjectHeader = NULL;
        PKIX_PL_EqualsCallback func = NULL;
        pkix_ClassTable_Entry entry;
        PKIX_UInt32 objType;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        PKIX_CHECK(pkix_pl_Object_GetHeader
                    (firstObject, &firstObjectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        PKIX_CHECK(pkix_pl_Object_GetHeader
                    (secondObject, &secondObjectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        
        if (firstObjectHeader->hashcodeCached &&
            secondObjectHeader->hashcodeCached){
                if (firstObjectHeader->hashcode !=
                    secondObjectHeader->hashcode){
                        *pResult = PKIX_FALSE;
                        goto cleanup;
                }
        }

        objType = firstObjectHeader->type;

        if (objType >= PKIX_NUMTYPES) {
#ifdef PKIX_USER_OBJECT_TYPE
                pkix_ClassTable_Entry *ctEntry = NULL;
                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                        (classTable,
                        (void *)&firstObjectHeader->type,
                        firstObjectHeader->type,
                        NULL,
                        (void **)&ctEntry,
                        plContext);
                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);

                if (pkixErrorResult){
                        PKIX_ERROR_FATAL(PKIX_ERRORGETTINGCLASSTABLEENTRY);
                }

                if ((ctEntry == NULL) || (ctEntry->equalsFunction == NULL)) {
                        PKIX_ERROR_FATAL(PKIX_UNDEFINEDCALLBACK);
                } else {
                        func = ctEntry->equalsFunction;
                }
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        } else {
                entry = systemClasses[objType];
                func = entry.equalsFunction;
                if (func == NULL){
                        func = pkix_pl_Object_Equals_Default;
                }
        }

        PKIX_CHECK(func(firstObject, secondObject, pResult, plContext),
                    PKIX_OBJECTSPECIFICFUNCTIONFAILED);

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_Duplicate(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_PL_Object *firstObjectHeader = NULL;
        PKIX_PL_DuplicateCallback func = NULL;
        pkix_ClassTable_Entry entry;
        PKIX_UInt32 objType;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Duplicate");
        PKIX_NULLCHECK_TWO(firstObject, pNewObject);

        PKIX_CHECK(pkix_pl_Object_GetHeader
                    (firstObject, &firstObjectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        objType = firstObjectHeader->type;

        if (objType >= PKIX_NUMTYPES) {
#ifdef PKIX_USER_OBJECT_TYPE
                pkix_ClassTable_Entry *ctEntry = NULL;

                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                        (classTable,
                        (void *)&objType,
                        objType,
                        NULL,
                        (void **)&ctEntry,
                        plContext);
                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);

                if (pkixErrorResult){
                        PKIX_ERROR_FATAL(PKIX_ERRORGETTINGCLASSTABLEENTRY);
                }

                if ((ctEntry == NULL) || (ctEntry->duplicateFunction == NULL)) {
                        PKIX_ERROR_FATAL(PKIX_UNDEFINEDCALLBACK);
                } else {
                        func = ctEntry->duplicateFunction;
                }
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        } else {
                entry = systemClasses[objType];
                func = entry.duplicateFunction;
                if (!func){
                        PKIX_ERROR_FATAL(PKIX_UNDEFINEDDUPLICATEFUNCTION);
                }
        }

        PKIX_CHECK(func(firstObject, pNewObject, plContext),
                    PKIX_OBJECTSPECIFICFUNCTIONFAILED);

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pValue,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;
        PKIX_PL_HashcodeCallback func = NULL;
        pkix_ClassTable_Entry entry;
        PKIX_UInt32 objectHash;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Hashcode");
        PKIX_NULLCHECK_TWO(object, pValue);

        
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        
        if (!objectHeader->hashcodeCached){

                PKIX_UInt32 objType = objectHeader->type;

                
                if (objType >= PKIX_NUMTYPES){
#ifdef PKIX_USER_OBJECT_TYPE            
                        pkix_ClassTable_Entry *ctEntry = NULL;

                        PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                        PR_Lock(classTableLock);
                        pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                                (classTable,
                                (void *)&objType,
                                objType,
                                NULL,
                                (void **)&ctEntry,
                                plContext);
                        PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                        PR_Unlock(classTableLock);

                        if (pkixErrorResult){
                                PKIX_ERROR_FATAL
                                        (PKIX_ERRORGETTINGCLASSTABLEENTRY);
                        }

                        if ((ctEntry == NULL) ||
                            (ctEntry->hashcodeFunction == NULL)) {
                                PKIX_ERROR_FATAL(PKIX_UNDEFINEDCALLBACK);
                        }

                        func = ctEntry->hashcodeFunction;
#else
                        PORT_Assert (0);
                        pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                        pkixErrorClass = PKIX_FATAL_ERROR;
                        goto cleanup;
#endif 
                } else {
                        entry = systemClasses[objType];
                        func = entry.hashcodeFunction;
                        if (func == NULL){
                                func = pkix_pl_Object_Hashcode_Default;
                        }
                }

                PKIX_CHECK(func(object, &objectHash, plContext),
                            PKIX_OBJECTSPECIFICFUNCTIONFAILED);

                if (!objectHeader->hashcodeCached){

                        PKIX_CHECK(pkix_LockObject(object, plContext),
                                    PKIX_ERRORLOCKINGOBJECT);

                        if (!objectHeader->hashcodeCached){
                                
                                objectHeader->hashcode = objectHash;
                                objectHeader->hashcodeCached = PKIX_TRUE;
                        }

                        PKIX_CHECK(pkix_UnlockObject(object, plContext),
                                    PKIX_ERRORUNLOCKINGOBJECT);
                }
        }

        *pValue = objectHeader->hashcode;

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;
        PKIX_PL_ToStringCallback func = NULL;
        pkix_ClassTable_Entry entry;
        PKIX_PL_String *objectString = NULL;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_ToString");
        PKIX_NULLCHECK_TWO(object, pString);

        
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        
        if (!objectHeader->stringRep){

                PKIX_UInt32 objType = objectHeader->type;

                if (objType >= PKIX_NUMTYPES){
#ifdef PKIX_USER_OBJECT_TYPE
                        pkix_ClassTable_Entry *ctEntry = NULL;

                        PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                        PR_Lock(classTableLock);
                        pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                                (classTable,
                                (void *)&objType,
                                objType,
                                NULL,
                                (void **)&ctEntry,
                                plContext);
                        PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                        PR_Unlock(classTableLock);
                        if (pkixErrorResult){
                                PKIX_ERROR_FATAL
                                        (PKIX_ERRORGETTINGCLASSTABLEENTRY);
                        }

                        if ((ctEntry == NULL) ||
                            (ctEntry->toStringFunction == NULL)) {
                                PKIX_ERROR_FATAL(PKIX_UNDEFINEDCALLBACK);
                        }

                        func = ctEntry->toStringFunction;
#else
                        PORT_Assert (0);
                        pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                        pkixErrorClass = PKIX_FATAL_ERROR;
                        goto cleanup;
#endif 
                } else {
                        entry = systemClasses[objType];
                        func = entry.toStringFunction;
                        if (func == NULL){
                                func = pkix_pl_Object_ToString_Default;
                        }
                }

                PKIX_CHECK(func(object, &objectString, plContext),
                            PKIX_OBJECTSPECIFICFUNCTIONFAILED);

                if (!objectHeader->stringRep){

                        PKIX_CHECK(pkix_LockObject(object, plContext),
                                    PKIX_ERRORLOCKINGOBJECT);

                        if (!objectHeader->stringRep){
                                
                                objectHeader->stringRep = objectString;
                                objectString = NULL;
                        }

                        PKIX_CHECK(pkix_UnlockObject(object, plContext),
                                    PKIX_ERRORUNLOCKINGOBJECT);
                }
        }


        *pString = objectHeader->stringRep;
        objectHeader->stringRep = NULL;

cleanup:
        if (objectHeader) {
            PKIX_DECREF(objectHeader->stringRep);
        }
        PKIX_DECREF(objectString);

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_InvalidateCache(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_InvalidateCache");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        PKIX_CHECK(pkix_LockObject(object, plContext),
                    PKIX_ERRORLOCKINGOBJECT);

        
        objectHeader->hashcode = 0;
        objectHeader->hashcodeCached = PKIX_FALSE;

        PKIX_DECREF(objectHeader->stringRep);

        PKIX_CHECK(pkix_UnlockObject(object, plContext),
                    PKIX_ERRORUNLOCKINGOBJECT);

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_Compare(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_Object *firstObjectHeader = NULL;
        PKIX_PL_Object *secondObjectHeader = NULL;
        PKIX_PL_ComparatorCallback func = NULL;
        pkix_ClassTable_Entry entry;
        PKIX_UInt32 objType;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Compare");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_pl_Object_GetHeader
                    (firstObject, &firstObjectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        
        PKIX_CHECK(pkix_pl_Object_GetHeader
                    (secondObject, &secondObjectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        objType = firstObjectHeader->type;

        if (objType >= PKIX_NUMTYPES){
#ifdef PKIX_USER_OBJECT_TYPE
                pkix_ClassTable_Entry *ctEntry = NULL;

                PKIX_OBJECT_DEBUG("\tCalling PR_Lock).\n");
                PR_Lock(classTableLock);
                pkixErrorResult = pkix_pl_PrimHashTable_Lookup
                        (classTable,
                        (void *)&objType,
                        objType,
                        NULL,
                        (void **)&ctEntry,
                        plContext);
                PKIX_OBJECT_DEBUG("\tCalling PR_Unlock).\n");
                PR_Unlock(classTableLock);
                if (pkixErrorResult){
                        PKIX_ERROR_FATAL(PKIX_ERRORGETTINGCLASSTABLEENTRY);
                }

                if ((ctEntry == NULL) || (ctEntry->comparator == NULL)) {
                        PKIX_ERROR_FATAL(PKIX_UNDEFINEDCOMPARATOR);
                }

                func = ctEntry->comparator;
#else
                PORT_Assert (0);
                pkixErrorCode = PKIX_UNKNOWNOBJECTTYPE;
                pkixErrorClass = PKIX_FATAL_ERROR;
                goto cleanup;
#endif 
        } else {
                
                entry = systemClasses[objType];
                func = entry.comparator;
                if (!func){
                        PKIX_ERROR(PKIX_UNDEFINEDCOMPARATOR);
                }
        }

        PKIX_CHECK(func(firstObject, secondObject, pResult, plContext),
                    PKIX_OBJECTSPECIFICFUNCTIONFAILED);

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_Lock(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Lock");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_LockObject(object, plContext),
                    PKIX_LOCKOBJECTFAILED);

cleanup:

        PKIX_RETURN(OBJECT);
}




PKIX_Error *
PKIX_PL_Object_Unlock(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_ENTER(OBJECT, "PKIX_PL_Object_Unlock");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_UnlockObject(object, plContext),
                    PKIX_UNLOCKOBJECTFAILED);

cleanup:

        PKIX_RETURN(OBJECT);
}





PKIX_Error *
PKIX_PL_Object_GetType(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pType,
        void *plContext)
{
        PKIX_PL_Object *objectHeader = NULL;

        PKIX_ENTER(OBJECT, "PKIX_PL_Object_GetType");
        PKIX_NULLCHECK_TWO(object, pType);

        
        PKIX_CHECK(pkix_pl_Object_GetHeader(object, &objectHeader, plContext),
                    PKIX_RECEIVEDCORRUPTEDOBJECTARGUMENT);

        *pType = objectHeader->type;

cleanup:

        PKIX_RETURN(OBJECT);
}
