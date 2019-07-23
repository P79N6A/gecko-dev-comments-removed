










































#include "pkix_revocationchecker.h"







static PKIX_Error *
pkix_RevocationChecker_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_RevocationChecker *checker = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_RevocationChecker_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_REVOCATIONCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTREVOCATIONCHECKER);

        checker = (PKIX_RevocationChecker *)object;

        PKIX_DECREF(checker->revCheckerContext);

cleanup:

        PKIX_RETURN(REVOCATIONCHECKER);
}





static PKIX_Error *
pkix_RevocationChecker_Duplicate(
        PKIX_PL_Object *object,
        PKIX_PL_Object **pNewObject,
        void *plContext)
{
        PKIX_RevocationChecker *checker = NULL;
        PKIX_RevocationChecker *checkerDuplicate = NULL;
        PKIX_PL_Object *contextDuplicate = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_RevocationChecker_Duplicate");
        PKIX_NULLCHECK_TWO(object, pNewObject);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_REVOCATIONCHECKER_TYPE, plContext),
                    PKIX_OBJECTNOTCERTCHAINCHECKER);

        checker = (PKIX_RevocationChecker *)object;

        if (checker->revCheckerContext){
                PKIX_CHECK(PKIX_PL_Object_Duplicate
                            ((PKIX_PL_Object *)checker->revCheckerContext,
                            (PKIX_PL_Object **)&contextDuplicate,
                            plContext),
                            PKIX_OBJECTDUPLICATEFAILED);
        }

        PKIX_CHECK(PKIX_RevocationChecker_Create
                    (checker->checkCallback,
                    contextDuplicate,
                    &checkerDuplicate,
                    plContext),
                    PKIX_REVOCATIONCHECKERCREATEFAILED);

        *pNewObject = (PKIX_PL_Object *)checkerDuplicate;

cleanup:

        PKIX_DECREF(contextDuplicate);

        PKIX_RETURN(REVOCATIONCHECKER);
}













PKIX_Error *
pkix_RevocationChecker_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(REVOCATIONCHECKER, "pkix_RevocationChecker_RegisterSelf");

        entry.description = "RevocationChecker";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_RevocationChecker);
        entry.destructor = pkix_RevocationChecker_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_RevocationChecker_Duplicate;

        systemClasses[PKIX_REVOCATIONCHECKER_TYPE] = entry;

        PKIX_RETURN(REVOCATIONCHECKER);
}







PKIX_Error *
PKIX_RevocationChecker_Create(
    PKIX_RevocationChecker_RevCallback callback,
    PKIX_PL_Object *revCheckerContext,
    PKIX_RevocationChecker **pChecker,
    void *plContext)
{
        PKIX_RevocationChecker *checker = NULL;

        PKIX_ENTER(REVOCATIONCHECKER, "PKIX_RevocationChecker_Create");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_REVOCATIONCHECKER_TYPE,
                    sizeof (PKIX_RevocationChecker),
                    (PKIX_PL_Object **)&checker,
                    plContext),
                    PKIX_COULDNOTCREATECERTCHAINCHECKEROBJECT);

        
        checker->checkCallback = callback;

        PKIX_INCREF(revCheckerContext);
        checker->revCheckerContext = revCheckerContext;

        *pChecker = checker;

cleanup:

        PKIX_RETURN(REVOCATIONCHECKER);

}





PKIX_Error *
PKIX_RevocationChecker_GetRevCallback(
        PKIX_RevocationChecker *checker,
        PKIX_RevocationChecker_RevCallback *pCallback,
        void *plContext)
{
        PKIX_ENTER
                (REVOCATIONCHECKER, "PKIX_RevocationChecker_GetRevCallback");
        PKIX_NULLCHECK_TWO(checker, pCallback);

        *pCallback = checker->checkCallback;

        PKIX_RETURN(REVOCATIONCHECKER);
}





PKIX_Error *
PKIX_RevocationChecker_GetRevCheckerContext(
        PKIX_RevocationChecker *checker,
        PKIX_PL_Object **pRevCheckerContext,
        void *plContext)
{
        PKIX_ENTER(REVOCATIONCHECKER,
                    "PKIX_RevocationChecker_GetRevCheckerContext");

        PKIX_NULLCHECK_TWO(checker, pRevCheckerContext);

        PKIX_INCREF(checker->revCheckerContext);

        *pRevCheckerContext = checker->revCheckerContext;

cleanup:
        PKIX_RETURN(REVOCATIONCHECKER);

}
