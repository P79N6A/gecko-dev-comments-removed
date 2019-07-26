









#include "pkix_pl_nameconstraints.h"




























static PKIX_Error *
pkix_pl_CertNameConstraints_GetPermitted(
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_List **pPermittedList,
        void *plContext)
{
        CERTNameConstraints *nssNameConstraints = NULL;
        CERTNameConstraints **nssNameConstraintsList = NULL;
        CERTNameConstraint *nssPermitted = NULL;
        CERTNameConstraint *firstPermitted = NULL;
        PKIX_List *permittedList = NULL;
        PKIX_PL_GeneralName *name = NULL;
        PKIX_UInt32 numItems = 0;
        PKIX_UInt32 i;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                "pkix_pl_CertNameConstraints_GetPermitted");
        PKIX_NULLCHECK_TWO(nameConstraints, pPermittedList);

        





        if (nameConstraints->permittedList == NULL) {

            PKIX_OBJECT_LOCK(nameConstraints);

            if (nameConstraints->permittedList == NULL) {

                PKIX_CHECK(PKIX_List_Create(&permittedList, plContext),
                        PKIX_LISTCREATEFAILED);

                numItems = nameConstraints->numNssNameConstraints;
                nssNameConstraintsList =
                        nameConstraints->nssNameConstraintsList;

                for (i = 0; i < numItems; i++) {

                    PKIX_NULLCHECK_ONE(nssNameConstraintsList);
                    nssNameConstraints = *(nssNameConstraintsList + i);
                    PKIX_NULLCHECK_ONE(nssNameConstraints);

                    if (nssNameConstraints->permited != NULL) {

                        nssPermitted = nssNameConstraints->permited;
                        firstPermitted = nssPermitted;

                        do {

                            PKIX_CHECK(pkix_pl_GeneralName_Create
                                (&nssPermitted->name, &name, plContext),
                                PKIX_GENERALNAMECREATEFAILED);

                            PKIX_CHECK(PKIX_List_AppendItem
                                (permittedList,
                                (PKIX_PL_Object *)name,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                            PKIX_DECREF(name);

                            PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling CERT_GetNextNameConstraint\n");
                            nssPermitted = CERT_GetNextNameConstraint
                                (nssPermitted);

                        } while (nssPermitted != firstPermitted);

                    }
                }

                PKIX_CHECK(PKIX_List_SetImmutable(permittedList, plContext),
                            PKIX_LISTSETIMMUTABLEFAILED);

                nameConstraints->permittedList = permittedList;

            }

            PKIX_OBJECT_UNLOCK(nameConstraints);

        }

        PKIX_INCREF(nameConstraints->permittedList);

        *pPermittedList = nameConstraints->permittedList;

cleanup:

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}

























static PKIX_Error *
pkix_pl_CertNameConstraints_GetExcluded(
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_List **pExcludedList,
        void *plContext)
{
        CERTNameConstraints *nssNameConstraints = NULL;
        CERTNameConstraints **nssNameConstraintsList = NULL;
        CERTNameConstraint *nssExcluded = NULL;
        CERTNameConstraint *firstExcluded = NULL;
        PKIX_List *excludedList = NULL;
        PKIX_PL_GeneralName *name = NULL;
        PKIX_UInt32 numItems = 0;
        PKIX_UInt32 i;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                "pkix_pl_CertNameConstraints_GetExcluded");
        PKIX_NULLCHECK_TWO(nameConstraints, pExcludedList);

        if (nameConstraints->excludedList == NULL) {

            PKIX_OBJECT_LOCK(nameConstraints);

            if (nameConstraints->excludedList == NULL) {

                PKIX_CHECK(PKIX_List_Create(&excludedList, plContext),
                            PKIX_LISTCREATEFAILED);

                numItems = nameConstraints->numNssNameConstraints;
                nssNameConstraintsList =
                        nameConstraints->nssNameConstraintsList;

                for (i = 0; i < numItems; i++) {

                    PKIX_NULLCHECK_ONE(nssNameConstraintsList);
                    nssNameConstraints = *(nssNameConstraintsList + i);
                    PKIX_NULLCHECK_ONE(nssNameConstraints);

                    if (nssNameConstraints->excluded != NULL) {

                        nssExcluded = nssNameConstraints->excluded;
                        firstExcluded = nssExcluded;

                        do {

                            PKIX_CHECK(pkix_pl_GeneralName_Create
                                (&nssExcluded->name, &name, plContext),
                                PKIX_GENERALNAMECREATEFAILED);

                            PKIX_CHECK(PKIX_List_AppendItem
                                (excludedList,
                                (PKIX_PL_Object *)name,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);

                            PKIX_DECREF(name);

                            PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling CERT_GetNextNameConstraint\n");
                            nssExcluded = CERT_GetNextNameConstraint
                                (nssExcluded);

                        } while (nssExcluded != firstExcluded);

                    }

                }
                PKIX_CHECK(PKIX_List_SetImmutable(excludedList, plContext),
                            PKIX_LISTSETIMMUTABLEFAILED);

                nameConstraints->excludedList = excludedList;

            }

            PKIX_OBJECT_UNLOCK(nameConstraints);
        }

        PKIX_INCREF(nameConstraints->excludedList);

        *pExcludedList = nameConstraints->excludedList;

cleanup:

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}





























PKIX_Error *
pkix_pl_CertNameConstraints_CheckNameSpaceNssNames(
        CERTGeneralName *nssSubjectNames,
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_Boolean *pCheckPass,
        void *plContext)
{
        CERTNameConstraints **nssNameConstraintsList = NULL;
        CERTNameConstraints *nssNameConstraints = NULL;
        CERTGeneralName *nssMatchName = NULL;
        PRArenaPool *arena = NULL;
        PKIX_UInt32 numItems = 0;
        PKIX_UInt32 i;
        SECStatus status = SECSuccess;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                "pkix_pl_CertNameConstraints_CheckNameSpaceNssNames");
        PKIX_NULLCHECK_THREE(nssSubjectNames, nameConstraints, pCheckPass);

        *pCheckPass = PKIX_TRUE;

        PKIX_CERTNAMECONSTRAINTS_DEBUG("\t\tCalling PORT_NewArena\n");
        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (arena == NULL) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        nssMatchName = nssSubjectNames;
        nssNameConstraintsList = nameConstraints->nssNameConstraintsList;

        












        do {

            numItems = nameConstraints->numNssNameConstraints;

            for (i = 0; i < numItems; i++) {

                PKIX_NULLCHECK_ONE(nssNameConstraintsList);
                nssNameConstraints = *(nssNameConstraintsList + i);
                PKIX_NULLCHECK_ONE(nssNameConstraints);

                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling CERT_CheckNameSpace\n");
                status = CERT_CheckNameSpace
                        (arena, nssNameConstraints, nssMatchName);
                if (status != SECSuccess) {
                        break;
                }

            }

            if (status != SECSuccess) {
                    break;
            }

            PKIX_CERTNAMECONSTRAINTS_DEBUG
                    ("\t\tCalling CERT_GetNextGeneralName\n");
            nssMatchName = CERT_GetNextGeneralName(nssMatchName);

        } while (nssMatchName != nssSubjectNames);

        if (status == SECFailure) {

                *pCheckPass = PKIX_FALSE;
        }

cleanup:

        if (arena){
                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling PORT_FreeArena).\n");
                PORT_FreeArena(arena, PR_FALSE);
        }

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertNameConstraints_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTS, "pkix_pl_CertNameConstraints_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTNAMECONSTRAINTS_TYPE, plContext),
                PKIX_OBJECTNOTCERTNAMECONSTRAINTS);

        nameConstraints = (PKIX_PL_CertNameConstraints *)object;

        PKIX_CHECK(PKIX_PL_Free
                    (nameConstraints->nssNameConstraintsList, plContext),
                    PKIX_FREEFAILED);

        if (nameConstraints->arena){
                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling PORT_FreeArena).\n");
                PORT_FreeArena(nameConstraints->arena, PR_FALSE);
                nameConstraints->arena = NULL;
        }

        PKIX_DECREF(nameConstraints->permittedList);
        PKIX_DECREF(nameConstraints->excludedList);

cleanup:

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}























static PKIX_Error *
pkix_pl_CertNameConstraints_ToString_Helper(
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_PL_String **pString,
        void *plContext)
{
        char *asciiFormat = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_List *permittedList = NULL;
        PKIX_List *excludedList = NULL;
        PKIX_PL_String *permittedListString = NULL;
        PKIX_PL_String *excludedListString = NULL;
        PKIX_PL_String *nameConstraintsString = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                    "pkix_pl_CertNameConstraints_ToString_Helper");
        PKIX_NULLCHECK_TWO(nameConstraints, pString);

        asciiFormat =
                "[\n"
                "\t\tPermitted Name:  %s\n"
                "\t\tExcluded Name:   %s\n"
                "\t]\n";

        PKIX_CHECK(PKIX_PL_String_Create
                    (PKIX_ESCASCII,
                    asciiFormat,
                    0,
                    &formatString,
                    plContext),
                    PKIX_STRINGCREATEFAILED);

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetPermitted
                    (nameConstraints, &permittedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETPERMITTEDFAILED);

        PKIX_TOSTRING(permittedList, &permittedListString, plContext,
                    PKIX_LISTTOSTRINGFAILED);

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetExcluded
                    (nameConstraints, &excludedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETEXCLUDEDFAILED);

        PKIX_TOSTRING(excludedList, &excludedListString, plContext,
                    PKIX_LISTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                    (&nameConstraintsString,
                    plContext,
                    formatString,
                    permittedListString,
                    excludedListString),
                    PKIX_SPRINTFFAILED);

        *pString = nameConstraintsString;

cleanup:

        PKIX_DECREF(formatString);
        PKIX_DECREF(permittedList);
        PKIX_DECREF(excludedList);
        PKIX_DECREF(permittedListString);
        PKIX_DECREF(excludedListString);

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertNameConstraints_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_PL_String *nameConstraintsString = NULL;
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTS, "pkix_pl_CertNameConstraints_ToString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType(
                    object, PKIX_CERTNAMECONSTRAINTS_TYPE, plContext),
                    PKIX_OBJECTNOTCERTNAMECONSTRAINTS);

        nameConstraints = (PKIX_PL_CertNameConstraints *)object;

        PKIX_CHECK(pkix_pl_CertNameConstraints_ToString_Helper
                    (nameConstraints, &nameConstraintsString, plContext),
                    PKIX_CERTNAMECONSTRAINTSTOSTRINGHELPERFAILED);

        *pString = nameConstraintsString;

cleanup:

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertNameConstraints_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        PKIX_List *permittedList = NULL;
        PKIX_List *excludedList = NULL;
        PKIX_UInt32 permitHash = 0;
        PKIX_UInt32 excludeHash = 0;

        PKIX_ENTER(CERTNAMECONSTRAINTS, "pkix_pl_CertNameConstraints_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_CERTNAMECONSTRAINTS_TYPE, plContext),
                    PKIX_OBJECTNOTCERTNAMECONSTRAINTS);

        nameConstraints = (PKIX_PL_CertNameConstraints *)object;

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetPermitted
                    (nameConstraints, &permittedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETPERMITTEDFAILED);

        PKIX_HASHCODE(permittedList, &permitHash, plContext,
                    PKIX_OBJECTHASHCODEFAILED);

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetExcluded
                    (nameConstraints, &excludedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETEXCLUDEDFAILED);

        PKIX_HASHCODE(excludedList, &excludeHash, plContext,
                    PKIX_OBJECTHASHCODEFAILED);

        *pHashcode = (((permitHash << 7) + excludeHash) << 7) +
                nameConstraints->numNssNameConstraints;

cleanup:

        PKIX_DECREF(permittedList);
        PKIX_DECREF(excludedList);
        PKIX_RETURN(CERTNAMECONSTRAINTS);
}





static PKIX_Error *
pkix_pl_CertNameConstraints_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Boolean *pResult,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *firstNC = NULL;
        PKIX_PL_CertNameConstraints *secondNC = NULL;
        PKIX_List *firstPermittedList = NULL;
        PKIX_List *secondPermittedList = NULL;
        PKIX_List *firstExcludedList = NULL;
        PKIX_List *secondExcludedList = NULL;
        PKIX_UInt32 secondType;
        PKIX_Boolean cmpResult = PKIX_FALSE;

        PKIX_ENTER(CERTNAMECONSTRAINTS, "pkix_pl_CertNameConstraints_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        
        PKIX_CHECK(pkix_CheckType
                (firstObject, PKIX_CERTNAMECONSTRAINTS_TYPE, plContext),
                PKIX_FIRSTOBJECTNOTCERTNAMECONSTRAINTS);

        firstNC = (PKIX_PL_CertNameConstraints *)firstObject;
        secondNC = (PKIX_PL_CertNameConstraints *)secondObject;

        



        if (firstNC == secondNC){
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        



        *pResult = PKIX_FALSE;

        PKIX_CHECK(PKIX_PL_Object_GetType
                    ((PKIX_PL_Object *)secondNC, &secondType, plContext),
                    PKIX_COULDNOTGETTYPEOFSECONDARGUMENT);

        if (secondType != PKIX_CERTNAMECONSTRAINTS_TYPE) {
                goto cleanup;
        }

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetPermitted
                    (firstNC, &firstPermittedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETPERMITTEDFAILED);

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetPermitted
                    (secondNC, &secondPermittedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETPERMITTEDFAILED);

        PKIX_EQUALS
                (firstPermittedList, secondPermittedList, &cmpResult, plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (cmpResult != PKIX_TRUE) {
                goto cleanup;
        }

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetExcluded
                    (firstNC, &firstExcludedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETEXCLUDEDFAILED);

        PKIX_CHECK(pkix_pl_CertNameConstraints_GetExcluded
                    (secondNC, &secondExcludedList, plContext),
                    PKIX_CERTNAMECONSTRAINTSGETEXCLUDEDFAILED);

        PKIX_EQUALS
                (firstExcludedList, secondExcludedList, &cmpResult, plContext,
                PKIX_OBJECTEQUALSFAILED);

        if (cmpResult != PKIX_TRUE) {
                goto cleanup;
        }

        




        *pResult = PKIX_TRUE;

cleanup:

        PKIX_DECREF(firstPermittedList);
        PKIX_DECREF(secondPermittedList);
        PKIX_DECREF(firstExcludedList);
        PKIX_DECREF(secondExcludedList);

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}













PKIX_Error *
pkix_pl_CertNameConstraints_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                    "pkix_pl_CertNameConstraints_RegisterSelf");

        entry.description = "CertNameConstraints";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_CertNameConstraints);
        entry.destructor = pkix_pl_CertNameConstraints_Destroy;
        entry.equalsFunction = pkix_pl_CertNameConstraints_Equals;
        entry.hashcodeFunction = pkix_pl_CertNameConstraints_Hashcode;
        entry.toStringFunction = pkix_pl_CertNameConstraints_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = pkix_duplicateImmutable;

        systemClasses[PKIX_CERTNAMECONSTRAINTS_TYPE] = entry;

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}

























static PKIX_Error *
pkix_pl_CertNameConstraints_Create_Helper(
        CERTNameConstraints *nssNameConstraints,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        CERTNameConstraints **nssNameConstraintPtr = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                    "pkix_pl_CertNameConstraints_Create_Helper");
        PKIX_NULLCHECK_TWO(nssNameConstraints, pNameConstraints);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_CERTNAMECONSTRAINTS_TYPE,
                    sizeof (PKIX_PL_CertNameConstraints),
                    (PKIX_PL_Object **)&nameConstraints,
                    plContext),
                    PKIX_COULDNOTCREATECERTNAMECONSTRAINTSOBJECT);

        PKIX_CHECK(PKIX_PL_Malloc
                    (sizeof (CERTNameConstraint *),
                    (void *)&nssNameConstraintPtr,
                    plContext),
                    PKIX_MALLOCFAILED);

        nameConstraints->numNssNameConstraints = 1;
        nameConstraints->nssNameConstraintsList = nssNameConstraintPtr;
        *nssNameConstraintPtr = nssNameConstraints;

        nameConstraints->permittedList = NULL;
        nameConstraints->excludedList = NULL;
        nameConstraints->arena = NULL;

        *pNameConstraints = nameConstraints;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(nameConstraints);
        }

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}
























PKIX_Error *
pkix_pl_CertNameConstraints_Create(
        CERTCertificate *nssCert,
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        CERTNameConstraints *nssNameConstraints = NULL;
        PLArenaPool *arena = NULL;
        SECStatus status;

        PKIX_ENTER(CERTNAMECONSTRAINTS, "pkix_pl_CertNameConstraints_Create");
        PKIX_NULLCHECK_THREE(nssCert, pNameConstraints, nssCert->arena);

        PKIX_CERTNAMECONSTRAINTS_DEBUG("\t\tCalling PORT_NewArena).\n");
        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (arena == NULL) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        PKIX_CERTNAMECONSTRAINTS_DEBUG
                ("\t\tCalling CERT_FindNameConstraintsExten\n");
        status = CERT_FindNameConstraintsExten
                (arena, nssCert, &nssNameConstraints);

        if (status != SECSuccess) {
                PKIX_ERROR(PKIX_DECODINGCERTNAMECONSTRAINTSFAILED);
        }

        if (nssNameConstraints == NULL) {
                *pNameConstraints = NULL;
                if (arena){
                        PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling PORT_FreeArena).\n");
                        PORT_FreeArena(arena, PR_FALSE);
                }
                goto cleanup;
        }

        PKIX_CHECK(pkix_pl_CertNameConstraints_Create_Helper
                    (nssNameConstraints, &nameConstraints, plContext),
                    PKIX_CERTNAMECONSTRAINTSCREATEHELPERFAILED);

        nameConstraints->arena = arena;

        *pNameConstraints = nameConstraints;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                if (arena){
                        PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling PORT_FreeArena).\n");
                        PORT_FreeArena(arena, PR_FALSE);
                }
        }

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}
























static PKIX_Error *
pkix_pl_CertNameConstraints_CreateByMerge(
        PKIX_PL_CertNameConstraints **pNameConstraints,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        CERTNameConstraints *nssNameConstraints = NULL;
        PLArenaPool *arena = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                    "pkix_pl_CertNameConstraints_CreateByMerge");
        PKIX_NULLCHECK_ONE(pNameConstraints);

        PKIX_CERTNAMECONSTRAINTS_DEBUG("\t\tCalling PORT_NewArena).\n");
        arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
        if (arena == NULL) {
                PKIX_ERROR(PKIX_OUTOFMEMORY);
        }

        PKIX_CERTNAMECONSTRAINTS_DEBUG("\t\tCalling PORT_ArenaZNew).\n");
        nssNameConstraints = PORT_ArenaZNew(arena, CERTNameConstraints);
        if (nssNameConstraints == NULL) {
                PKIX_ERROR(PKIX_PORTARENAALLOCFAILED);
        }

        nssNameConstraints->permited = NULL;
        nssNameConstraints->excluded = NULL;
        nssNameConstraints->DERPermited = NULL;
        nssNameConstraints->DERExcluded = NULL;

        PKIX_CHECK(pkix_pl_CertNameConstraints_Create_Helper
                    (nssNameConstraints, &nameConstraints, plContext),
                    PKIX_CERTNAMECONSTRAINTSCREATEHELPERFAILED);

        nameConstraints->arena = arena;

        *pNameConstraints = nameConstraints;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                if (arena){
                        PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling PORT_FreeArena).\n");
                        PORT_FreeArena(arena, PR_FALSE);
                }
        }

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}





























static PKIX_Error *
pkix_pl_CertNameConstraints_CopyNssNameConstraints(
        PLArenaPool *arena,
        CERTNameConstraints *srcNC,
        CERTNameConstraints **pDestNC,
        void *plContext)
{
        CERTNameConstraints *nssNameConstraints = NULL;
        CERTNameConstraint *nssNameConstraintHead = NULL;
        CERTNameConstraint *nssCurrent = NULL;
        CERTNameConstraint *nssCopyTo = NULL;
        CERTNameConstraint *nssCopyFrom = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                    "pkix_pl_CertNameConstraints_CopyNssNameConstraints");
        PKIX_NULLCHECK_THREE(arena, srcNC, pDestNC);

        PKIX_CERTNAMECONSTRAINTS_DEBUG("\t\tCalling PORT_ArenaZNew).\n");
        nssNameConstraints = PORT_ArenaZNew(arena, CERTNameConstraints);
        if (nssNameConstraints == NULL) {
                PKIX_ERROR(PKIX_PORTARENAALLOCFAILED);
        }

        if (srcNC->permited) {

            nssCopyFrom = srcNC->permited;

            do {

                nssCopyTo = NULL;
                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling CERT_CopyNameConstraint).\n");
                nssCopyTo = CERT_CopyNameConstraint
                        (arena, nssCopyTo, nssCopyFrom);
                if (nssCopyTo == NULL) {
                        PKIX_ERROR(PKIX_CERTCOPYNAMECONSTRAINTFAILED);
                }
                if (nssCurrent == NULL) {
                        nssCurrent = nssNameConstraintHead = nssCopyTo;
                } else {
                        PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling CERT_AddNameConstraint).\n");
                        nssCurrent = CERT_AddNameConstraint
                                (nssCurrent, nssCopyTo);
                }

                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling CERT_GetNextNameConstrain).\n");
                nssCopyFrom = CERT_GetNextNameConstraint(nssCopyFrom);

            } while (nssCopyFrom != srcNC->permited);

            nssNameConstraints->permited = nssNameConstraintHead;
        }

        if (srcNC->excluded) {

            nssCurrent = NULL;
            nssCopyFrom = srcNC->excluded;

            do {

                





                nssCopyTo = NULL;
                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling CERT_CopyNameConstraint).\n");
                nssCopyTo = CERT_CopyNameConstraint
                        (arena, nssCopyTo, nssCopyFrom);
                if (nssCopyTo == NULL) {
                        PKIX_ERROR(PKIX_CERTCOPYNAMECONSTRAINTFAILED);
                }
                if (nssCurrent == NULL) {
                        nssCurrent = nssNameConstraintHead = nssCopyTo;
                } else {
                        PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling CERT_AddNameConstraint).\n");
                        nssCurrent = CERT_AddNameConstraint
                                (nssCurrent, nssCopyTo);
                }

                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling CERT_GetNextNameConstrain).\n");
                nssCopyFrom = CERT_GetNextNameConstraint(nssCopyFrom);

            } while (nssCopyFrom != srcNC->excluded);

            nssNameConstraints->excluded = nssNameConstraintHead;
        }

        *pDestNC = nssNameConstraints;

cleanup:

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}


























PKIX_Error *
pkix_pl_CertNameConstraints_Merge(
        PKIX_PL_CertNameConstraints *firstNC,
        PKIX_PL_CertNameConstraints *secondNC,
        PKIX_PL_CertNameConstraints **pMergedNC,
        void *plContext)
{
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        CERTNameConstraints **nssNCto = NULL;
        CERTNameConstraints **nssNCfrom = NULL;
        CERTNameConstraints *nssNameConstraints = NULL;
        PKIX_UInt32 numNssItems = 0;
        PKIX_UInt32 i;

        PKIX_ENTER(CERTNAMECONSTRAINTS, "pkix_pl_CertNameConstraints_Merge");
        PKIX_NULLCHECK_THREE(firstNC, secondNC, pMergedNC);

        PKIX_CHECK(pkix_pl_CertNameConstraints_CreateByMerge
                    (&nameConstraints, plContext),
                    PKIX_CERTNAMECONSTRAINTSCREATEBYMERGEFAILED);

        

        numNssItems = firstNC->numNssNameConstraints +
                    secondNC->numNssNameConstraints;

        
        PKIX_CHECK(PKIX_PL_Free
                    (nameConstraints->nssNameConstraintsList, plContext),
                    PKIX_FREEFAILED);

        
        PKIX_CHECK(PKIX_PL_Malloc
                    (numNssItems * sizeof (CERTNameConstraint *),
                    (void *)&nssNCto,
                    plContext),
                    PKIX_MALLOCFAILED);

        nameConstraints->nssNameConstraintsList = nssNCto;

        nssNCfrom = firstNC->nssNameConstraintsList;

        for (i = 0; i < firstNC->numNssNameConstraints; i++) {

                PKIX_CHECK(pkix_pl_CertNameConstraints_CopyNssNameConstraints
                        (nameConstraints->arena,
                        *nssNCfrom,
                        &nssNameConstraints,
                        plContext),
                        PKIX_CERTNAMECONSTRAINTSCOPYNSSNAMECONSTRAINTSFAILED);

                *nssNCto = nssNameConstraints;

                nssNCto++;
                nssNCfrom++;
        }

        nssNCfrom = secondNC->nssNameConstraintsList;

        for (i = 0; i < secondNC->numNssNameConstraints; i++) {

                PKIX_CHECK(pkix_pl_CertNameConstraints_CopyNssNameConstraints
                        (nameConstraints->arena,
                        *nssNCfrom,
                        &nssNameConstraints,
                        plContext),
                        PKIX_CERTNAMECONSTRAINTSCOPYNSSNAMECONSTRAINTSFAILED);

                *nssNCto = nssNameConstraints;

                nssNCto++;
                nssNCfrom++;
        }

        nameConstraints->numNssNameConstraints = numNssItems;
        nameConstraints->permittedList = NULL;
        nameConstraints->excludedList = NULL;

        *pMergedNC = nameConstraints;

cleanup:

        if (PKIX_ERROR_RECEIVED){
                PKIX_DECREF(nameConstraints);
        }

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}







PKIX_Error *
PKIX_PL_CertNameConstraints_CheckNamesInNameSpace(
        PKIX_List *nameList, 
        PKIX_PL_CertNameConstraints *nameConstraints,
        PKIX_Boolean *pCheckPass,
        void *plContext)
{
        CERTNameConstraints **nssNameConstraintsList = NULL;
        CERTNameConstraints *nssNameConstraints = NULL;
        CERTGeneralName *nssMatchName = NULL;
        PRArenaPool *arena = NULL;
        PKIX_PL_GeneralName *name = NULL;
        PKIX_UInt32 numNameItems = 0;
        PKIX_UInt32 numNCItems = 0;
        PKIX_UInt32 i, j;
        SECStatus status = SECSuccess;

        PKIX_ENTER(CERTNAMECONSTRAINTS,
                "PKIX_PL_CertNameConstraints_CheckNamesInNameSpace");
        PKIX_NULLCHECK_TWO(nameConstraints, pCheckPass);

        *pCheckPass = PKIX_TRUE;

        if (nameList != NULL) {

                PKIX_CERTNAMECONSTRAINTS_DEBUG("\t\tCalling PORT_NewArena\n");
                arena = PORT_NewArena(DER_DEFAULT_CHUNKSIZE);
                if (arena == NULL) {
                        PKIX_ERROR(PKIX_OUTOFMEMORY);
                }

                nssNameConstraintsList =
                        nameConstraints->nssNameConstraintsList;
                PKIX_NULLCHECK_ONE(nssNameConstraintsList);
                numNCItems = nameConstraints->numNssNameConstraints;

                PKIX_CHECK(PKIX_List_GetLength
                        (nameList, &numNameItems, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                for (i = 0; i < numNameItems; i++) {

                        PKIX_CHECK(PKIX_List_GetItem
                                (nameList,
                                i,
                                (PKIX_PL_Object **) &name,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK(pkix_pl_GeneralName_GetNssGeneralName
                                (name, &nssMatchName, plContext),
                                PKIX_GENERALNAMEGETNSSGENERALNAMEFAILED);

                        PKIX_DECREF(name);

                        for (j = 0; j < numNCItems; j++) {

                            nssNameConstraints = *(nssNameConstraintsList + j);
                            PKIX_NULLCHECK_ONE(nssNameConstraints);

                            PKIX_CERTNAMECONSTRAINTS_DEBUG
                                ("\t\tCalling CERT_CheckNameSpace\n");
                            status = CERT_CheckNameSpace
                                (arena, nssNameConstraints, nssMatchName);
                            if (status != SECSuccess) {
                                break;
                            }

                        }

                        if (status != SECSuccess) {
                            break;
                        }

                }
        }

        if (status == SECFailure) {
                *pCheckPass = PKIX_FALSE;
        }

cleanup:

        if (arena){
                PKIX_CERTNAMECONSTRAINTS_DEBUG
                        ("\t\tCalling PORT_FreeArena).\n");
                PORT_FreeArena(arena, PR_FALSE);
        }

        PKIX_RETURN(CERTNAMECONSTRAINTS);
}
