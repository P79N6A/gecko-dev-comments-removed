









































#include "pkix_policychecker.h"



static PKIX_Error *
pkix_PolicyChecker_MakeSingleton(
        PKIX_PL_Object *listItem,
        PKIX_Boolean immutability,
        PKIX_List **pList,
        void *plContext);







static PKIX_Error *
pkix_PolicyCheckerState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PolicyCheckerState *checkerState = NULL;

        PKIX_ENTER(CERTPOLICYCHECKERSTATE, "pkix_PolicyCheckerState_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYCHECKERSTATE_TYPE, plContext),
                PKIX_OBJECTNOTPOLICYCHECKERSTATE);

        checkerState = (PKIX_PolicyCheckerState *)object;

        PKIX_DECREF(checkerState->certPoliciesExtension);
        PKIX_DECREF(checkerState->policyMappingsExtension);
        PKIX_DECREF(checkerState->policyConstraintsExtension);
        PKIX_DECREF(checkerState->inhibitAnyPolicyExtension);
        PKIX_DECREF(checkerState->anyPolicyOID);
        PKIX_DECREF(checkerState->validPolicyTree);
        PKIX_DECREF(checkerState->userInitialPolicySet);
        PKIX_DECREF(checkerState->mappedUserInitialPolicySet);

        checkerState->policyQualifiersRejected = PKIX_FALSE;
        checkerState->explicitPolicy = 0;
        checkerState->inhibitAnyPolicy = 0;
        checkerState->policyMapping = 0;
        checkerState->numCerts = 0;
        checkerState->certsProcessed = 0;
        checkerState->certPoliciesCritical = PKIX_FALSE;

        PKIX_DECREF(checkerState->anyPolicyNodeAtBottom);
        PKIX_DECREF(checkerState->newAnyPolicyNode);
        PKIX_DECREF(checkerState->mappedPolicyOIDs);

cleanup:

        PKIX_RETURN(CERTPOLICYCHECKERSTATE);
}





static PKIX_Error *
pkix_PolicyCheckerState_ToString(
        PKIX_PL_Object *object,
        PKIX_PL_String **pCheckerStateString,
        void *plContext)
{
        PKIX_PolicyCheckerState *state = NULL;
        PKIX_PL_String *resultString = NULL;
        PKIX_PL_String *policiesExtOIDString = NULL;
        PKIX_PL_String *policyMapOIDString = NULL;
        PKIX_PL_String *policyConstrOIDString = NULL;
        PKIX_PL_String *inhAnyPolOIDString = NULL;
        PKIX_PL_String *anyPolicyOIDString = NULL;
        PKIX_PL_String *validPolicyTreeString = NULL;
        PKIX_PL_String *userInitialPolicySetString = NULL;
        PKIX_PL_String *mappedUserPolicySetString = NULL;
        PKIX_PL_String *mappedPolicyOIDsString = NULL;
        PKIX_PL_String *anyAtBottomString = NULL;
        PKIX_PL_String *newAnyPolicyString = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *trueString = NULL;
        PKIX_PL_String *falseString = NULL;
        PKIX_PL_String *nullString = NULL;
        PKIX_Boolean initialPolicyMappingInhibit = PKIX_FALSE;
        PKIX_Boolean initialExplicitPolicy = PKIX_FALSE;
        PKIX_Boolean initialAnyPolicyInhibit = PKIX_FALSE;
        PKIX_Boolean initialIsAnyPolicy = PKIX_FALSE;
        PKIX_Boolean policyQualifiersRejected = PKIX_FALSE;
        PKIX_Boolean certPoliciesCritical = PKIX_FALSE;
        char *asciiFormat =
                "{\n"
                "\tcertPoliciesExtension:    \t%s\n"
                "\tpolicyMappingsExtension:  \t%s\n"
                "\tpolicyConstraintsExtension:\t%s\n"
                "\tinhibitAnyPolicyExtension:\t%s\n"
                "\tanyPolicyOID:             \t%s\n"
                "\tinitialIsAnyPolicy:       \t%s\n"
                "\tvalidPolicyTree:          \t%s\n"
                "\tuserInitialPolicySet:     \t%s\n"
                "\tmappedUserPolicySet:      \t%s\n"
                "\tpolicyQualifiersRejected: \t%s\n"
                "\tinitialPolMappingInhibit: \t%s\n"
                "\tinitialExplicitPolicy:    \t%s\n"
                "\tinitialAnyPolicyInhibit:  \t%s\n"
                "\texplicitPolicy:           \t%d\n"
                "\tinhibitAnyPolicy:         \t%d\n"
                "\tpolicyMapping:            \t%d\n"
                "\tnumCerts:                 \t%d\n"
                "\tcertsProcessed:           \t%d\n"
                "\tanyPolicyNodeAtBottom:    \t%s\n"
                "\tnewAnyPolicyNode:         \t%s\n"
                "\tcertPoliciesCritical:     \t%s\n"
                "\tmappedPolicyOIDs:         \t%s\n"
                "}";

        PKIX_ENTER(CERTPOLICYCHECKERSTATE, "pkix_PolicyCheckerState_ToString");

        PKIX_NULLCHECK_TWO(object, pCheckerStateString);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_CERTPOLICYCHECKERSTATE_TYPE, plContext),
                PKIX_OBJECTNOTPOLICYCHECKERSTATE);

        state = (PKIX_PolicyCheckerState *)object;
        PKIX_NULLCHECK_THREE
                (state->certPoliciesExtension,
                state->policyMappingsExtension,
                state->policyConstraintsExtension);
        PKIX_NULLCHECK_THREE
                (state->inhibitAnyPolicyExtension,
                state->anyPolicyOID,
                state->userInitialPolicySet);

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, asciiFormat, 0, &formatString, plContext),
                PKIX_STRINGCREATEFAILED);
        




        initialPolicyMappingInhibit = state->initialPolicyMappingInhibit;
        initialExplicitPolicy = state->initialExplicitPolicy;
        initialAnyPolicyInhibit = state->initialAnyPolicyInhibit;
        initialIsAnyPolicy = state->initialIsAnyPolicy;
        policyQualifiersRejected = state->policyQualifiersRejected;
        certPoliciesCritical = state->certPoliciesCritical;

        if (initialPolicyMappingInhibit || initialExplicitPolicy ||
            initialAnyPolicyInhibit || initialIsAnyPolicy ||
            policyQualifiersRejected || certPoliciesCritical) {
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII, "TRUE", 0, &trueString, plContext),
                        PKIX_STRINGCREATEFAILED);
        }
        if (!initialPolicyMappingInhibit || !initialExplicitPolicy ||
            !initialAnyPolicyInhibit || !initialIsAnyPolicy ||
            !policyQualifiersRejected || !certPoliciesCritical) {
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII, "FALSE", 0, &falseString, plContext),
                        PKIX_STRINGCREATEFAILED);
        }
        if (!(state->anyPolicyNodeAtBottom) || !(state->newAnyPolicyNode)) {
                PKIX_CHECK(PKIX_PL_String_Create
                        (PKIX_ESCASCII, "(null)", 0, &nullString, plContext),
                        PKIX_STRINGCREATEFAILED);
        }

        PKIX_TOSTRING
                (state->certPoliciesExtension, &policiesExtOIDString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->policyMappingsExtension,
                &policyMapOIDString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->policyConstraintsExtension,
                &policyConstrOIDString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->inhibitAnyPolicyExtension,
                &inhAnyPolOIDString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING(state->anyPolicyOID, &anyPolicyOIDString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING(state->validPolicyTree, &validPolicyTreeString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->userInitialPolicySet,
                &userInitialPolicySetString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->mappedUserInitialPolicySet,
                &mappedUserPolicySetString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        if (state->anyPolicyNodeAtBottom) {
                PKIX_CHECK(pkix_SinglePolicyNode_ToString
                        (state->anyPolicyNodeAtBottom,
                        &anyAtBottomString,
                        plContext),
                        PKIX_SINGLEPOLICYNODETOSTRINGFAILED);
        } else {
                PKIX_INCREF(nullString);
                anyAtBottomString = nullString;
        }

        if (state->newAnyPolicyNode) {
                PKIX_CHECK(pkix_SinglePolicyNode_ToString
                        (state->newAnyPolicyNode,
                        &newAnyPolicyString,
                        plContext),
                        PKIX_SINGLEPOLICYNODETOSTRINGFAILED);
        } else {
                PKIX_INCREF(nullString);
                newAnyPolicyString = nullString;
        }

        PKIX_TOSTRING
                (state->mappedPolicyOIDs,
                &mappedPolicyOIDsString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&resultString,
                plContext,
                formatString,
                policiesExtOIDString,
                policyMapOIDString,
                policyConstrOIDString,
                inhAnyPolOIDString,
                anyPolicyOIDString,
                initialIsAnyPolicy?trueString:falseString,
                validPolicyTreeString,
                userInitialPolicySetString,
                mappedUserPolicySetString,
                policyQualifiersRejected?trueString:falseString,
                initialPolicyMappingInhibit?trueString:falseString,
                initialExplicitPolicy?trueString:falseString,
                initialAnyPolicyInhibit?trueString:falseString,
                state->explicitPolicy,
                state->inhibitAnyPolicy,
                state->policyMapping,
                state->numCerts,
                state->certsProcessed,
                anyAtBottomString,
                newAnyPolicyString,
                certPoliciesCritical?trueString:falseString,
                mappedPolicyOIDsString),
                PKIX_SPRINTFFAILED);

        *pCheckerStateString = resultString;

cleanup:
        PKIX_DECREF(policiesExtOIDString);
        PKIX_DECREF(policyMapOIDString);
        PKIX_DECREF(policyConstrOIDString);
        PKIX_DECREF(inhAnyPolOIDString);
        PKIX_DECREF(anyPolicyOIDString);
        PKIX_DECREF(validPolicyTreeString);
        PKIX_DECREF(userInitialPolicySetString);
        PKIX_DECREF(mappedUserPolicySetString);
        PKIX_DECREF(anyAtBottomString);
        PKIX_DECREF(newAnyPolicyString);
        PKIX_DECREF(mappedPolicyOIDsString);
        PKIX_DECREF(formatString);
        PKIX_DECREF(trueString);
        PKIX_DECREF(falseString);
        PKIX_DECREF(nullString);

        PKIX_RETURN(CERTPOLICYCHECKERSTATE);
}


















PKIX_Error *
pkix_PolicyCheckerState_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER
                (CERTPOLICYCHECKERSTATE,
                "pkix_PolicyCheckerState_RegisterSelf");

        entry.description = "PolicyCheckerState";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PolicyCheckerState);
        entry.destructor = pkix_PolicyCheckerState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = pkix_PolicyCheckerState_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_CERTPOLICYCHECKERSTATE_TYPE] = entry;

        PKIX_RETURN(CERTPOLICYCHECKERSTATE);
}









































static PKIX_Error *
pkix_PolicyCheckerState_Create(
        PKIX_List *initialPolicies,
        PKIX_Boolean policyQualifiersRejected,
        PKIX_Boolean initialPolicyMappingInhibit,
        PKIX_Boolean initialExplicitPolicy,
        PKIX_Boolean initialAnyPolicyInhibit,
        PKIX_UInt32 numCerts,
        PKIX_PolicyCheckerState **pCheckerState,
        void *plContext)
{
        PKIX_PolicyCheckerState *checkerState = NULL;
        PKIX_PolicyNode *policyNode = NULL;
        PKIX_List *anyPolicyList = NULL;

        PKIX_ENTER(CERTPOLICYCHECKERSTATE, "pkix_PolicyCheckerState_Create");
        PKIX_NULLCHECK_TWO(initialPolicies, pCheckerState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_CERTPOLICYCHECKERSTATE_TYPE,
                sizeof (PKIX_PolicyCheckerState),
                (PKIX_PL_Object **)&checkerState,
                plContext),
                PKIX_COULDNOTCREATEPOLICYCHECKERSTATEOBJECT);

        

        PKIX_CHECK(PKIX_PL_OID_Create
                (PKIX_CERTIFICATEPOLICIES_OID,
                &(checkerState->certPoliciesExtension),
                plContext),
                PKIX_OIDCREATEFAILED);

        PKIX_CHECK(PKIX_PL_OID_Create
                (PKIX_POLICYMAPPINGS_OID,
                &(checkerState->policyMappingsExtension),
                plContext),
                PKIX_OIDCREATEFAILED);

        PKIX_CHECK(PKIX_PL_OID_Create
                (PKIX_POLICYCONSTRAINTS_OID,
                &(checkerState->policyConstraintsExtension),
                plContext),
                PKIX_OIDCREATEFAILED);

        PKIX_CHECK(PKIX_PL_OID_Create
                (PKIX_INHIBITANYPOLICY_OID,
                &(checkerState->inhibitAnyPolicyExtension),
                plContext),
                PKIX_OIDCREATEFAILED);

        PKIX_CHECK(PKIX_PL_OID_Create
                (PKIX_CERTIFICATEPOLICIES_ANYPOLICY_OID,
                &(checkerState->anyPolicyOID),
                plContext),
                PKIX_OIDCREATEFAILED);

        
        PKIX_INCREF(initialPolicies);
        checkerState->userInitialPolicySet = initialPolicies;
        PKIX_INCREF(initialPolicies);
        checkerState->mappedUserInitialPolicySet = initialPolicies;

        PKIX_CHECK(pkix_List_Contains
                (initialPolicies,
                (PKIX_PL_Object *)(checkerState->anyPolicyOID),
                &(checkerState->initialIsAnyPolicy),
                plContext),
                PKIX_LISTCONTAINSFAILED);

        checkerState->policyQualifiersRejected =
                policyQualifiersRejected;
        checkerState->initialExplicitPolicy = initialExplicitPolicy;
        checkerState->explicitPolicy =
                (initialExplicitPolicy? 0: numCerts + 1);
        checkerState->initialAnyPolicyInhibit = initialAnyPolicyInhibit;
        checkerState->inhibitAnyPolicy =
                (initialAnyPolicyInhibit? 0: numCerts + 1);
        checkerState->initialPolicyMappingInhibit = initialPolicyMappingInhibit;
        checkerState->policyMapping =
                (initialPolicyMappingInhibit? 0: numCerts + 1);
                ;
        checkerState->numCerts = numCerts;
        checkerState->certsProcessed = 0;
        checkerState->certPoliciesCritical = PKIX_FALSE;

        
        PKIX_CHECK(pkix_PolicyChecker_MakeSingleton
                ((PKIX_PL_Object *)(checkerState->anyPolicyOID),
                PKIX_TRUE,
                &anyPolicyList,
                plContext),
                PKIX_POLICYCHECKERMAKESINGLETONFAILED);

        PKIX_CHECK(pkix_PolicyNode_Create
                (checkerState->anyPolicyOID,    
                NULL,                           
                PKIX_FALSE,                     
                anyPolicyList,                  
                &policyNode,
                plContext),
                PKIX_POLICYNODECREATEFAILED);
        checkerState->validPolicyTree = policyNode;

        



        PKIX_INCREF(policyNode);
        checkerState->anyPolicyNodeAtBottom = policyNode;

        checkerState->newAnyPolicyNode = NULL;

        checkerState->mappedPolicyOIDs = NULL;

        *pCheckerState = checkerState;
        checkerState = NULL;

cleanup:

        PKIX_DECREF(checkerState);

        PKIX_DECREF(anyPolicyList);

        PKIX_RETURN(CERTPOLICYCHECKERSTATE);
}

































PKIX_Error *
pkix_PolicyChecker_MapContains(
        PKIX_List *certPolicyMaps,
        PKIX_PL_OID *policy,
        PKIX_Boolean *pFound,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *map = NULL;
        PKIX_UInt32 numEntries = 0;
        PKIX_UInt32 index = 0;
        PKIX_Boolean match = PKIX_FALSE;
        PKIX_PL_OID *issuerDomainPolicy = NULL;
        PKIX_PL_OID *subjectDomainPolicy = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_MapContains");
        PKIX_NULLCHECK_THREE(certPolicyMaps, policy, pFound);

        PKIX_CHECK(PKIX_List_GetLength(certPolicyMaps, &numEntries, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (index = 0; (!match) && (index < numEntries); index++) {
                PKIX_CHECK(PKIX_List_GetItem
                    (certPolicyMaps, index, (PKIX_PL_Object **)&map, plContext),
                    PKIX_LISTGETITEMFAILED);

                PKIX_NULLCHECK_ONE(map);

                PKIX_CHECK(PKIX_PL_CertPolicyMap_GetIssuerDomainPolicy
                        (map, &issuerDomainPolicy, plContext),
                        PKIX_CERTPOLICYMAPGETISSUERDOMAINPOLICYFAILED);

                PKIX_EQUALS
                        (policy, issuerDomainPolicy, &match, plContext,
                        PKIX_OBJECTEQUALSFAILED);

                if (!match) {
                        PKIX_CHECK(PKIX_PL_CertPolicyMap_GetSubjectDomainPolicy
                                (map, &subjectDomainPolicy, plContext),
                                PKIX_CERTPOLICYMAPGETSUBJECTDOMAINPOLICYFAILED);

                        PKIX_EQUALS
                                (policy, subjectDomainPolicy, &match, plContext,
                                PKIX_OBJECTEQUALSFAILED);
                }

                PKIX_DECREF(map);
                PKIX_DECREF(issuerDomainPolicy);
                PKIX_DECREF(subjectDomainPolicy);
        }

        *pFound = match;

cleanup:

        PKIX_DECREF(map);
        PKIX_DECREF(issuerDomainPolicy);
        PKIX_DECREF(subjectDomainPolicy);
        PKIX_RETURN(CERTCHAINCHECKER);
}

































PKIX_Error *
pkix_PolicyChecker_MapGetSubjectDomainPolicies(
        PKIX_List *certPolicyMaps,
        PKIX_PL_OID *policy,
        PKIX_List **pSubjectDomainPolicies,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *map = NULL;
        PKIX_List *subjectList = NULL;
        PKIX_UInt32 numEntries = 0;
        PKIX_UInt32 index = 0;
        PKIX_Boolean match = PKIX_FALSE;
        PKIX_PL_OID *issuerDomainPolicy = NULL;
        PKIX_PL_OID *subjectDomainPolicy = NULL;

        PKIX_ENTER
                (CERTCHAINCHECKER,
                "pkix_PolicyChecker_MapGetSubjectDomainPolicies");
        PKIX_NULLCHECK_TWO(policy, pSubjectDomainPolicies);

        if (certPolicyMaps) {
                PKIX_CHECK(PKIX_List_GetLength
                    (certPolicyMaps,
                    &numEntries,
                    plContext),
                    PKIX_LISTGETLENGTHFAILED);
        }

        for (index = 0; index < numEntries; index++) {
                PKIX_CHECK(PKIX_List_GetItem
                    (certPolicyMaps, index, (PKIX_PL_Object **)&map, plContext),
                    PKIX_LISTGETITEMFAILED);

                PKIX_NULLCHECK_ONE(map);

                PKIX_CHECK(PKIX_PL_CertPolicyMap_GetIssuerDomainPolicy
                        (map, &issuerDomainPolicy, plContext),
                        PKIX_CERTPOLICYMAPGETISSUERDOMAINPOLICYFAILED);

                PKIX_EQUALS
                    (policy, issuerDomainPolicy, &match, plContext,
                    PKIX_OBJECTEQUALSFAILED);

                if (match) {
                    if (!subjectList) {
                        PKIX_CHECK(PKIX_List_Create(&subjectList, plContext),
                                PKIX_LISTCREATEFAILED);
                    }

                    PKIX_CHECK(PKIX_PL_CertPolicyMap_GetSubjectDomainPolicy
                        (map, &subjectDomainPolicy, plContext),
                        PKIX_CERTPOLICYMAPGETSUBJECTDOMAINPOLICYFAILED);

                    PKIX_CHECK(PKIX_List_AppendItem
                        (subjectList,
                        (PKIX_PL_Object *)subjectDomainPolicy,
                        plContext),
                        PKIX_LISTAPPENDITEMFAILED);
                }

                PKIX_DECREF(map);
                PKIX_DECREF(issuerDomainPolicy);
                PKIX_DECREF(subjectDomainPolicy);
        }

        if (subjectList) {
                PKIX_CHECK(PKIX_List_SetImmutable(subjectList, plContext),
                        PKIX_LISTSETIMMUTABLEFAILED);
        }

        *pSubjectDomainPolicies = subjectList;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(subjectList);
        }

        PKIX_DECREF(map);
        PKIX_DECREF(issuerDomainPolicy);
        PKIX_DECREF(subjectDomainPolicy);

        PKIX_RETURN(CERTCHAINCHECKER);
}





























PKIX_Error *
pkix_PolicyChecker_MapGetMappedPolicies(
        PKIX_List *certPolicyMaps,
        PKIX_List **pMappedPolicies,
        void *plContext)
{
        PKIX_PL_CertPolicyMap *map = NULL;
        PKIX_List *mappedList = NULL;
        PKIX_UInt32 numEntries = 0;
        PKIX_UInt32 index = 0;
        PKIX_Boolean isContained = PKIX_FALSE;
        PKIX_PL_OID *issuerDomainPolicy = NULL;

        PKIX_ENTER
                (CERTCHAINCHECKER, "pkix_PolicyChecker_MapGetMappedPolicies");
        PKIX_NULLCHECK_TWO(certPolicyMaps, pMappedPolicies);

        PKIX_CHECK(PKIX_List_Create(&mappedList, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_GetLength(certPolicyMaps, &numEntries, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (index = 0; index < numEntries; index++) {
                PKIX_CHECK(PKIX_List_GetItem
                    (certPolicyMaps, index, (PKIX_PL_Object **)&map, plContext),
                    PKIX_LISTGETITEMFAILED);

                PKIX_NULLCHECK_ONE(map);

                PKIX_CHECK(PKIX_PL_CertPolicyMap_GetIssuerDomainPolicy
                        (map, &issuerDomainPolicy, plContext),
                        PKIX_CERTPOLICYMAPGETISSUERDOMAINPOLICYFAILED);

                PKIX_CHECK(pkix_List_Contains
                        (mappedList,
                        (PKIX_PL_Object *)issuerDomainPolicy,
                        &isContained,
                        plContext),
                        PKIX_LISTCONTAINSFAILED);

                if (isContained == PKIX_FALSE) {
                        PKIX_CHECK(PKIX_List_AppendItem
                                (mappedList,
                                (PKIX_PL_Object *)issuerDomainPolicy,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);
                }

                PKIX_DECREF(map);
                PKIX_DECREF(issuerDomainPolicy);
        }

        *pMappedPolicies = mappedList;

cleanup:

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(mappedList);
        }

        PKIX_DECREF(map);
        PKIX_DECREF(issuerDomainPolicy);

        PKIX_RETURN(CERTCHAINCHECKER);
}






















static PKIX_Error *
pkix_PolicyChecker_MakeMutableCopy(
        PKIX_List *list,
        PKIX_List **pMutableCopy,
        void *plContext)
{
        PKIX_List *newList = NULL;
        PKIX_UInt32 listLen = 0;
        PKIX_UInt32 listIx = 0;
        PKIX_PL_Object *object;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_MakeMutableCopy");
        PKIX_NULLCHECK_TWO(list, pMutableCopy);

        PKIX_CHECK(PKIX_List_Create(&newList, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_GetLength(list, &listLen, plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (listIx = 0; listIx < listLen; listIx++) {

                PKIX_CHECK(PKIX_List_GetItem(list, listIx, &object, plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(PKIX_List_AppendItem(newList, object, plContext),
                        PKIX_LISTAPPENDITEMFAILED);

                PKIX_DECREF(object);
        }

        *pMutableCopy = newList;
        newList = NULL;
        
cleanup:
        PKIX_DECREF(newList);
        PKIX_DECREF(object);

        PKIX_RETURN(CERTCHAINCHECKER);
}

























static PKIX_Error *
pkix_PolicyChecker_MakeSingleton(
        PKIX_PL_Object *listItem,
        PKIX_Boolean immutability,
        PKIX_List **pList,
        void *plContext)
{
        PKIX_List *newList = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_MakeSingleton");
        PKIX_NULLCHECK_TWO(listItem, pList);

        PKIX_CHECK(PKIX_List_Create(&newList, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (newList, (PKIX_PL_Object *)listItem, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        if (immutability) {
                PKIX_CHECK(PKIX_List_SetImmutable(newList, plContext),
                        PKIX_LISTSETIMMUTABLEFAILED);
        }

        *pList = newList;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(newList);
        }

        PKIX_RETURN(CERTCHAINCHECKER);
}

















































static PKIX_Error *
pkix_PolicyChecker_Spawn(
        PKIX_PolicyNode *parent,
        PKIX_PL_OID *policyOID,
        PKIX_List *qualifiers,  
        PKIX_List *subjectDomainPolicies,
        PKIX_PolicyCheckerState *state,
        void *plContext)
{
        PKIX_List *expectedSet = NULL; 
        PKIX_PolicyNode *childNode = NULL;
        PKIX_Boolean match = PKIX_FALSE;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_Spawn");
        PKIX_NULLCHECK_THREE(policyOID, parent, state);

        if (subjectDomainPolicies) {

                PKIX_INCREF(subjectDomainPolicies);
                expectedSet = subjectDomainPolicies;

        } else {
                
                PKIX_CHECK(pkix_PolicyChecker_MakeSingleton
                        ((PKIX_PL_Object *)policyOID,
                        PKIX_TRUE,      
                        &expectedSet,
                        plContext),
                        PKIX_POLICYCHECKERMAKESINGLETONFAILED);
        }

        PKIX_CHECK(pkix_PolicyNode_Create
                (policyOID,
                qualifiers,
                state->certPoliciesCritical,
                expectedSet,
                &childNode,
                plContext),
                PKIX_POLICYNODECREATEFAILED);

        





        if (!subjectDomainPolicies) {
                PKIX_EQUALS(policyOID, state->anyPolicyOID, &match, plContext,
                        PKIX_OBJECTEQUALSFAILED);

                if (match) {
                        PKIX_DECREF(state->newAnyPolicyNode);
                        PKIX_INCREF(childNode);
                        state->newAnyPolicyNode = childNode;
                }
        }

        PKIX_CHECK(pkix_PolicyNode_AddToParent(parent, childNode, plContext),
                PKIX_POLICYNODEADDTOPARENTFAILED);

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)state, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

cleanup:
        PKIX_DECREF(childNode);
        PKIX_DECREF(expectedSet);
        PKIX_RETURN(CERTCHAINCHECKER);
}




















































static PKIX_Error *
pkix_PolicyChecker_CheckPolicyRecursive(
        PKIX_PL_OID *policyOID,
        PKIX_List *policyQualifiers,
        PKIX_List *subjectDomainPolicies,
        PKIX_PolicyNode *currentNode,
        PKIX_PolicyCheckerState *state,
        PKIX_Boolean *pChildNodeCreated,
        void *plContext)
{
        PKIX_UInt32 depth = 0;
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 childIx = 0;
        PKIX_Boolean isIncluded = PKIX_FALSE;
        PKIX_List *children = NULL;     
        PKIX_PolicyNode *childNode = NULL;
        PKIX_List *expectedPolicies = NULL; 

        PKIX_ENTER
                (CERTCHAINCHECKER,
                "pkix_PolicyChecker_CheckPolicyRecursive");
        PKIX_NULLCHECK_FOUR(policyOID, currentNode, state, pChildNodeCreated);

        
        PKIX_CHECK(PKIX_PolicyNode_GetDepth
                (currentNode, &depth, plContext),
                PKIX_POLICYNODEGETDEPTHFAILED);

        if (depth < (state->certsProcessed)) {
                PKIX_CHECK(pkix_PolicyNode_GetChildrenMutable
                        (currentNode, &children, plContext),
                        PKIX_POLICYNODEGETCHILDRENMUTABLEFAILED);

                if (children) {
                        PKIX_CHECK(PKIX_List_GetLength
                                (children, &numChildren, plContext),
                                PKIX_LISTGETLENGTHFAILED);
                }

                for (childIx = 0; childIx < numChildren; childIx++) {

                        PKIX_CHECK(PKIX_List_GetItem
                            (children,
                            childIx,
                            (PKIX_PL_Object **)&childNode,
                            plContext),
                            PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK(pkix_PolicyChecker_CheckPolicyRecursive
                            (policyOID,
                            policyQualifiers,
                            subjectDomainPolicies,
                            childNode,
                            state,
                            pChildNodeCreated,
                            plContext),
                            PKIX_POLICYCHECKERCHECKPOLICYRECURSIVEFAILED);

                        PKIX_DECREF(childNode);
                }
        } else { 

                
                PKIX_CHECK(PKIX_PolicyNode_GetExpectedPolicies
                        (currentNode, &expectedPolicies, plContext),
                        PKIX_POLICYNODEGETEXPECTEDPOLICIESFAILED);

                PKIX_NULLCHECK_ONE(expectedPolicies);

                PKIX_CHECK(pkix_List_Contains
                        (expectedPolicies,
                        (PKIX_PL_Object *)policyOID,
                        &isIncluded,
                        plContext),
                        PKIX_LISTCONTAINSFAILED);

                if (isIncluded) {
                        PKIX_CHECK(pkix_PolicyChecker_Spawn
                                (currentNode,
                                policyOID,
                                policyQualifiers,
                                subjectDomainPolicies,
                                state,
                                plContext),
                                PKIX_POLICYCHECKERSPAWNFAILED);

                        *pChildNodeCreated = PKIX_TRUE;
                }
        }

cleanup:

        PKIX_DECREF(children);
        PKIX_DECREF(childNode);
        PKIX_DECREF(expectedPolicies);

        PKIX_RETURN(CERTCHAINCHECKER);
}





































static PKIX_Error *
pkix_PolicyChecker_CheckPolicy(
        PKIX_PL_OID *policyOID,
        PKIX_List *policyQualifiers,
        PKIX_PL_Cert *cert,
        PKIX_List *maps,
        PKIX_PolicyCheckerState *state,
        void *plContext)
{
        PKIX_Boolean childNodeCreated = PKIX_FALSE;
        PKIX_Boolean okToSpawn = PKIX_FALSE;
        PKIX_Boolean found = PKIX_FALSE;
        PKIX_List *subjectDomainPolicies = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_CheckPolicy");
        PKIX_NULLCHECK_THREE(policyOID, cert, state);

        






        if (state->certsProcessed != (state->numCerts - 1)) {
            PKIX_CHECK(pkix_PolicyChecker_MapGetSubjectDomainPolicies
                (maps, policyOID, &subjectDomainPolicies, plContext),
                PKIX_POLICYCHECKERMAPGETSUBJECTDOMAINPOLICIESFAILED);
        }

        





        if ((state->policyMapping) == 0) {
                if (subjectDomainPolicies) {
                        goto cleanup;
                }
        }

        PKIX_CHECK(pkix_PolicyChecker_CheckPolicyRecursive
                (policyOID,
                policyQualifiers,
                subjectDomainPolicies,
                state->validPolicyTree,
                state,
                &childNodeCreated,
                plContext),
                PKIX_POLICYCHECKERCHECKPOLICYRECURSIVEFAILED);

        if (!childNodeCreated) {
                












                if (state->anyPolicyNodeAtBottom) {
                        if (state->initialIsAnyPolicy) {
                                okToSpawn = PKIX_TRUE;
                        } else {
                                PKIX_CHECK(pkix_List_Contains
                                        (state->mappedUserInitialPolicySet,
                                        (PKIX_PL_Object *)policyOID,
                                        &okToSpawn,
                                        plContext),
                                        PKIX_LISTCONTAINSFAILED);
                        }
                        if (okToSpawn) {
                                PKIX_CHECK(pkix_PolicyChecker_Spawn
                                        (state->anyPolicyNodeAtBottom,
                                        policyOID,
                                        policyQualifiers,
                                        subjectDomainPolicies,
                                        state,
                                        plContext),
                                        PKIX_POLICYCHECKERSPAWNFAILED);
                                childNodeCreated = PKIX_TRUE;
                        }
                }
        }

        if (childNodeCreated) {
                




                if (policyQualifiers &&
                    state->certPoliciesCritical &&
                    state->policyQualifiersRejected) {
                    PKIX_ERROR
                        (PKIX_QUALIFIERSINCRITICALCERTIFICATEPOLICYEXTENSION);
                }
                




                if (state->mappedPolicyOIDs) {
                        PKIX_CHECK(pkix_List_Contains
                                (state->mappedPolicyOIDs,
                                (PKIX_PL_Object *)policyOID,
                                &found,
                                plContext),
                                PKIX_LISTCONTAINSFAILED);
                        if (found) {
                                PKIX_CHECK(pkix_List_Remove
                                        (state->mappedPolicyOIDs,
                                        (PKIX_PL_Object *)policyOID,
                                        plContext),
                                        PKIX_LISTREMOVEFAILED);
                        }
                }
        }

cleanup:

        PKIX_DECREF(subjectDomainPolicies);

        PKIX_RETURN(CERTCHAINCHECKER);
}
















































static PKIX_Error *
pkix_PolicyChecker_CheckAny(
        PKIX_PolicyNode *currentNode,
        PKIX_List *qualsOfAny,  
        PKIX_List *policyMaps,  
        PKIX_PolicyCheckerState *state,
        void *plContext)
{
        PKIX_UInt32 depth = 0;
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 childIx = 0;
        PKIX_UInt32 numPolicies = 0;
        PKIX_UInt32 polx = 0;
        PKIX_Boolean isIncluded = PKIX_FALSE;
        PKIX_List *children = NULL;     
        PKIX_PolicyNode *childNode = NULL;
        PKIX_List *expectedPolicies = NULL; 
        PKIX_PL_OID *policyOID = NULL;
        PKIX_PL_OID *childPolicy = NULL;
        PKIX_List *subjectDomainPolicies = NULL;  

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_CheckAny");
        PKIX_NULLCHECK_TWO(currentNode, state);

        PKIX_CHECK(PKIX_PolicyNode_GetDepth
                (currentNode, &depth, plContext),
                PKIX_POLICYNODEGETDEPTHFAILED);

        PKIX_CHECK(pkix_PolicyNode_GetChildrenMutable
                (currentNode, &children, plContext),
                PKIX_POLICYNODEGETCHILDRENMUTABLEFAILED);

        if (children) {
                PKIX_CHECK(PKIX_List_GetLength
                        (children, &numChildren, plContext),
                        PKIX_LISTGETLENGTHFAILED);
        }

        if (depth < (state->certsProcessed)) {
                for (childIx = 0; childIx < numChildren; childIx++) {

                        PKIX_CHECK(PKIX_List_GetItem
                                (children,
                                childIx,
                                (PKIX_PL_Object **)&childNode,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                        PKIX_NULLCHECK_ONE(childNode);
                        PKIX_CHECK(pkix_PolicyChecker_CheckAny
                                (childNode,
                                qualsOfAny,
                                policyMaps,
                                state,
                                plContext),
                                PKIX_POLICYCHECKERCHECKANYFAILED);

                        PKIX_DECREF(childNode);
                }
        } else { 

            PKIX_CHECK(PKIX_PolicyNode_GetExpectedPolicies
                (currentNode, &expectedPolicies, plContext),
                PKIX_POLICYNODEGETEXPECTEDPOLICIESFAILED);

            
            PKIX_NULLCHECK_ONE(expectedPolicies);

            PKIX_CHECK(PKIX_List_GetLength
                (expectedPolicies, &numPolicies, plContext),
                PKIX_LISTGETLENGTHFAILED);

            for (polx = 0; polx < numPolicies; polx++) {
                PKIX_CHECK(PKIX_List_GetItem
                    (expectedPolicies,
                    polx,
                    (PKIX_PL_Object **)&policyOID,
                    plContext),
                    PKIX_LISTGETITEMFAILED);

                PKIX_NULLCHECK_ONE(policyOID);

                isIncluded = PKIX_FALSE;

                for (childIx = 0;
                    (!isIncluded && (childIx < numChildren));
                    childIx++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (children,
                        childIx,
                        (PKIX_PL_Object **)&childNode,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_NULLCHECK_ONE(childNode);

                    PKIX_CHECK(PKIX_PolicyNode_GetValidPolicy
                        (childNode, &childPolicy, plContext),
                        PKIX_POLICYNODEGETVALIDPOLICYFAILED);

                    PKIX_NULLCHECK_ONE(childPolicy);

                    PKIX_EQUALS(policyOID, childPolicy, &isIncluded, plContext,
                        PKIX_OBJECTEQUALSFAILED);

                    PKIX_DECREF(childNode);
                    PKIX_DECREF(childPolicy);
                }

                if (!isIncluded) {
                    if (policyMaps) {
                        PKIX_CHECK
                          (pkix_PolicyChecker_MapGetSubjectDomainPolicies
                          (policyMaps,
                          policyOID,
                          &subjectDomainPolicies,
                          plContext),
                          PKIX_POLICYCHECKERMAPGETSUBJECTDOMAINPOLICIESFAILED);
                    }
                    PKIX_CHECK(pkix_PolicyChecker_Spawn
                        (currentNode,
                        policyOID,
                        qualsOfAny,
                        subjectDomainPolicies,
                        state,
                        plContext),
                        PKIX_POLICYCHECKERSPAWNFAILED);
                    PKIX_DECREF(subjectDomainPolicies);
                }

                PKIX_DECREF(policyOID);
            }
        }

cleanup:

        PKIX_DECREF(children);
        PKIX_DECREF(childNode);
        PKIX_DECREF(expectedPolicies);
        PKIX_DECREF(policyOID);
        PKIX_DECREF(childPolicy);
        PKIX_DECREF(subjectDomainPolicies);

        PKIX_RETURN(CERTCHAINCHECKER);

}












































static PKIX_Error *
pkix_PolicyChecker_CalculateIntersection(
        PKIX_PolicyNode *currentNode,
        PKIX_PolicyCheckerState *state,
        PKIX_List *nominees, 
        PKIX_Boolean *pShouldBePruned,
        void *plContext)
{
        PKIX_Boolean currentPolicyIsAny = PKIX_FALSE;
        PKIX_Boolean parentPolicyIsAny = PKIX_FALSE;
        PKIX_Boolean currentPolicyIsValid = PKIX_FALSE;
        PKIX_Boolean shouldBePruned = PKIX_FALSE;
        PKIX_Boolean priorCriticality = PKIX_FALSE;
        PKIX_UInt32 depth = 0;
        PKIX_UInt32 numChildren = 0;
        PKIX_UInt32 childIndex = 0;
        PKIX_UInt32 numNominees = 0;
        PKIX_UInt32 polIx = 0;
        PKIX_PL_OID *currentPolicy = NULL;
        PKIX_PL_OID *parentPolicy = NULL;
        PKIX_PL_OID *substPolicy = NULL;
        PKIX_PolicyNode *parent = NULL;
        PKIX_PolicyNode *child = NULL;
        PKIX_List *children = NULL; 
        PKIX_List *policyQualifiers = NULL;

        PKIX_ENTER
                (CERTCHAINCHECKER,
                "pkix_PolicyChecker_CalculateIntersection");

        PKIX_NULLCHECK_FOUR(currentNode, state, nominees, pShouldBePruned);

        PKIX_CHECK(PKIX_PolicyNode_GetValidPolicy
                (currentNode, &currentPolicy, plContext),
                PKIX_POLICYNODEGETVALIDPOLICYFAILED);

        PKIX_NULLCHECK_TWO(state->anyPolicyOID, currentPolicy);

        PKIX_EQUALS
                (state->anyPolicyOID,
                currentPolicy,
                &currentPolicyIsAny,
                plContext,
                PKIX_OBJECTEQUALSFAILED);

        PKIX_CHECK(PKIX_PolicyNode_GetParent(currentNode, &parent, plContext),
                PKIX_POLICYNODEGETPARENTFAILED);

        if (currentPolicyIsAny == PKIX_FALSE) {

                




                if (parent) {
                        PKIX_CHECK(PKIX_PolicyNode_GetValidPolicy
                                (parent, &parentPolicy, plContext),
                                PKIX_POLICYNODEGETVALIDPOLICYFAILED);

                        PKIX_NULLCHECK_ONE(parentPolicy);

                        PKIX_EQUALS
                                (state->anyPolicyOID,
                                parentPolicy,
                                &parentPolicyIsAny,
                                plContext,
                                PKIX_OBJECTEQUALSFAILED);
                }

                




                if (!parent || parentPolicyIsAny) {
                        PKIX_CHECK(pkix_List_Contains
                                (state->userInitialPolicySet,
                                (PKIX_PL_Object *)currentPolicy,
                                &currentPolicyIsValid,
                                plContext),
                                PKIX_LISTCONTAINSFAILED);
                        if (!currentPolicyIsValid) {
                                *pShouldBePruned = PKIX_TRUE;
                                goto cleanup;
                        }

                        





                        PKIX_CHECK(pkix_List_Remove
                                (nominees,
                                (PKIX_PL_Object *)currentPolicy,
                                plContext),
                                PKIX_LISTREMOVEFAILED);
                }
        }


        

        PKIX_CHECK(PKIX_PolicyNode_GetDepth
                (currentNode, &depth, plContext),
                PKIX_POLICYNODEGETDEPTHFAILED);

        if (depth == (state->numCerts)) {
                



                if (currentPolicyIsAny == PKIX_TRUE) {

                        

                        PKIX_CHECK(PKIX_List_GetLength
                            (nominees, &numNominees, plContext),
                            PKIX_LISTGETLENGTHFAILED);

                        if (numNominees) {

                            PKIX_CHECK(PKIX_PolicyNode_GetPolicyQualifiers
                                (currentNode,
                                &policyQualifiers,
                                plContext),
                                PKIX_POLICYNODEGETPOLICYQUALIFIERSFAILED);

                            PKIX_CHECK(PKIX_PolicyNode_IsCritical
                                (currentNode, &priorCriticality, plContext),
                                PKIX_POLICYNODEISCRITICALFAILED);
                        }

                        PKIX_NULLCHECK_ONE(parent);

                        for (polIx = 0; polIx < numNominees; polIx++) {

                            PKIX_CHECK(PKIX_List_GetItem
                                (nominees,
                                polIx,
                                (PKIX_PL_Object **)&substPolicy,
                                plContext),
                                PKIX_LISTGETITEMFAILED);

                            PKIX_CHECK(pkix_PolicyChecker_Spawn
                                (parent,
                                substPolicy,
                                policyQualifiers,
                                NULL,
                                state,
                                plContext),
                                PKIX_POLICYCHECKERSPAWNFAILED);

                            PKIX_DECREF(substPolicy);

                        }
                        
                        *pShouldBePruned = PKIX_TRUE;
                        




                }
        } else {
                



                PKIX_CHECK(pkix_PolicyNode_GetChildrenMutable
                        (currentNode, &children, plContext),
                        PKIX_POLICYNODEGETCHILDRENMUTABLEFAILED);

                
                PKIX_NULLCHECK_ONE(children);

                PKIX_CHECK(PKIX_List_GetLength
                        (children, &numChildren, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                for (childIndex = numChildren; childIndex > 0; childIndex--) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (children,
                        childIndex - 1,
                        (PKIX_PL_Object **)&child,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK(pkix_PolicyChecker_CalculateIntersection
                        (child, state, nominees, &shouldBePruned, plContext),
                        PKIX_POLICYCHECKERCALCULATEINTERSECTIONFAILED);

                    if (PKIX_TRUE == shouldBePruned) {

                        PKIX_CHECK(PKIX_List_DeleteItem
                                (children, childIndex - 1, plContext),
                                PKIX_LISTDELETEITEMFAILED);
                        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                                ((PKIX_PL_Object *)state, plContext),
                                PKIX_OBJECTINVALIDATECACHEFAILED);
                    }

                    PKIX_DECREF(child);
                }

                PKIX_CHECK(PKIX_List_GetLength
                        (children, &numChildren, plContext),
                        PKIX_LISTGETLENGTHFAILED);

                if (numChildren == 0) {
                        *pShouldBePruned = PKIX_TRUE;
                }
        }
cleanup:
        PKIX_DECREF(currentPolicy);
        PKIX_DECREF(parentPolicy);
        PKIX_DECREF(substPolicy);
        PKIX_DECREF(parent);
        PKIX_DECREF(child);
        PKIX_DECREF(children);
        PKIX_DECREF(policyQualifiers);

        PKIX_RETURN(CERTCHAINCHECKER);

}



































static PKIX_Error *
pkix_PolicyChecker_PolicyMapProcessing(
        PKIX_List *policyMaps,  
        PKIX_Boolean certPoliciesIncludeAny,
        PKIX_List *qualsOfAny,
        PKIX_PolicyCheckerState *state,
        void *plContext)
{
        PKIX_UInt32 numPolicies = 0;
        PKIX_UInt32 polX = 0;
        PKIX_PL_OID *policyOID = NULL;
        PKIX_List *newMappedPolicies = NULL;  
        PKIX_List *subjectDomainPolicies = NULL;  

        PKIX_ENTER
                (CERTCHAINCHECKER,
                "pkix_PolicyChecker_PolicyMapProcessing");
        PKIX_NULLCHECK_THREE(policyMaps, state, state->userInitialPolicySet);

        





        PKIX_CHECK(PKIX_List_Create
                (&newMappedPolicies, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_GetLength
                (state->mappedUserInitialPolicySet,
                &numPolicies,
                plContext),
                PKIX_LISTGETLENGTHFAILED);

        for (polX = 0; polX < numPolicies; polX++) {

            PKIX_CHECK(PKIX_List_GetItem
                (state->mappedUserInitialPolicySet,
                polX,
                (PKIX_PL_Object **)&policyOID,
                plContext),
                PKIX_LISTGETITEMFAILED);

            PKIX_CHECK(pkix_PolicyChecker_MapGetSubjectDomainPolicies
                (policyMaps,
                policyOID,
                &subjectDomainPolicies,
                plContext),
                PKIX_POLICYCHECKERMAPGETSUBJECTDOMAINPOLICIESFAILED);

            if (subjectDomainPolicies) {

                PKIX_CHECK(pkix_List_AppendUnique
                        (newMappedPolicies,
                        subjectDomainPolicies,
                        plContext),
                        PKIX_LISTAPPENDUNIQUEFAILED);

                PKIX_DECREF(subjectDomainPolicies);

            } else {
                PKIX_CHECK(PKIX_List_AppendItem
                        (newMappedPolicies,
                        (PKIX_PL_Object *)policyOID,
                        plContext),
                        PKIX_LISTAPPENDITEMFAILED);
            }
            PKIX_DECREF(policyOID);
        }

        











        if ((state->policyMapping > 0) && (certPoliciesIncludeAny) &&
            (state->anyPolicyNodeAtBottom) && (state->mappedPolicyOIDs)) {

                PKIX_CHECK(PKIX_List_GetLength
                    (state->mappedPolicyOIDs,
                    &numPolicies,
                    plContext),
                    PKIX_LISTGETLENGTHFAILED);

                for (polX = 0; polX < numPolicies; polX++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (state->mappedPolicyOIDs,
                        polX,
                        (PKIX_PL_Object **)&policyOID,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK(pkix_PolicyChecker_MapGetSubjectDomainPolicies
                        (policyMaps,
                        policyOID,
                        &subjectDomainPolicies,
                        plContext),
                        PKIX_POLICYCHECKERMAPGETSUBJECTDOMAINPOLICIESFAILED);

                    PKIX_CHECK(pkix_PolicyChecker_Spawn
                        (state->anyPolicyNodeAtBottom,
                        policyOID,
                        qualsOfAny,
                        subjectDomainPolicies,
                        state,
                        plContext),
                        PKIX_POLICYCHECKERSPAWNFAILED);

                    PKIX_CHECK(pkix_List_AppendUnique
                        (newMappedPolicies,
                        subjectDomainPolicies,
                        plContext),
                        PKIX_LISTAPPENDUNIQUEFAILED);

                    PKIX_DECREF(subjectDomainPolicies);
                    PKIX_DECREF(policyOID);
                }
        }

        PKIX_CHECK(PKIX_List_SetImmutable(newMappedPolicies, plContext),
                PKIX_LISTSETIMMUTABLEFAILED);

        PKIX_DECREF(state->mappedUserInitialPolicySet);
        PKIX_INCREF(newMappedPolicies);

        state->mappedUserInitialPolicySet = newMappedPolicies;

cleanup:

        PKIX_DECREF(policyOID);
        PKIX_DECREF(newMappedPolicies);
        PKIX_DECREF(subjectDomainPolicies);

        PKIX_RETURN(CERTCHAINCHECKER);
}




























static PKIX_Error *
pkix_PolicyChecker_WrapUpProcessing(
        PKIX_PL_Cert *cert,
        PKIX_PolicyCheckerState *state,
        void *plContext)
{
        PKIX_Int32 explicitPolicySkipCerts = 0;
        PKIX_Boolean isSelfIssued = PKIX_FALSE;
        PKIX_Boolean shouldBePruned = PKIX_FALSE;
        PKIX_List *nominees = NULL; 
#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        PKIX_PL_String *stateString = NULL;
        char *stateAscii = NULL;
        PKIX_UInt32 length;
#endif

        PKIX_ENTER
                (CERTCHAINCHECKER,
                "pkix_PolicyChecker_WrapUpProcessing");
        PKIX_NULLCHECK_THREE(cert, state, state->userInitialPolicySet);

#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object*)state, &stateString, plContext),
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_String_GetEncoded
                    (stateString,
                    PKIX_ESCASCII,
                    (void **)&stateAscii,
                    &length,
                    plContext),
                    PKIX_STRINGGETENCODEDFAILED);

        PKIX_DEBUG_ARG("%s\n", stateAscii);

        PKIX_FREE(stateAscii);
        PKIX_DECREF(stateString);
#endif

        
        PKIX_CHECK(pkix_IsCertSelfIssued
                (cert, &isSelfIssued, plContext),
                PKIX_ISCERTSELFISSUEDFAILED);

        if (!isSelfIssued) {
                if (state->explicitPolicy > 0) {

                        state->explicitPolicy--;

                        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                                ((PKIX_PL_Object *)state, plContext),
                                PKIX_OBJECTINVALIDATECACHEFAILED);
                }
        }

        
        PKIX_CHECK(PKIX_PL_Cert_GetRequireExplicitPolicy
                (cert, &explicitPolicySkipCerts, plContext),
                PKIX_CERTGETREQUIREEXPLICITPOLICYFAILED);

        if (explicitPolicySkipCerts  == 0) {
                state->explicitPolicy = 0;
        }

        

        if (!(state->validPolicyTree)) {
                goto cleanup;
        }

        

        if (state->initialIsAnyPolicy) {
                goto cleanup;
        }

        




        PKIX_CHECK(pkix_PolicyChecker_MakeMutableCopy
                (state->userInitialPolicySet, &nominees, plContext),
                PKIX_POLICYCHECKERMAKEMUTABLECOPYFAILED);

        PKIX_CHECK(pkix_PolicyChecker_CalculateIntersection
                (state->validPolicyTree, 
                state,
                nominees,
                &shouldBePruned,
                plContext),
                PKIX_POLICYCHECKERCALCULATEINTERSECTIONFAILED);

        if (PKIX_TRUE == shouldBePruned) {
                PKIX_DECREF(state->validPolicyTree);
        }

        if (state->validPolicyTree) {
                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)state->validPolicyTree, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)state, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        if (state->validPolicyTree) {
                PKIX_CHECK(PKIX_PL_Object_ToString
                        ((PKIX_PL_Object*)state, &stateString, plContext),
                        PKIX_OBJECTTOSTRINGFAILED);

                PKIX_CHECK(PKIX_PL_String_GetEncoded
                            (stateString,
                            PKIX_ESCASCII,
                            (void **)&stateAscii,
                            &length,
                            plContext),
                            PKIX_STRINGGETENCODEDFAILED);

                PKIX_DEBUG_ARG
                        ("After CalculateIntersection:\n%s\n", stateAscii);

                PKIX_FREE(stateAscii);
                PKIX_DECREF(stateString);
        } else {
                PKIX_DEBUG("validPolicyTree is NULL\n");
        }
#endif

        

        if (state->validPolicyTree) {

                PKIX_CHECK(pkix_PolicyNode_Prune
                        (state->validPolicyTree,
                        state->numCerts,
                        &shouldBePruned,
                        plContext),
                        PKIX_POLICYNODEPRUNEFAILED);

                if (shouldBePruned) {
                        PKIX_DECREF(state->validPolicyTree);
                }
        }

        if (state->validPolicyTree) {
                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)state->validPolicyTree, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);
        }

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)state, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object*)state, &stateString, plContext),
                PKIX_OBJECTTOSTRINGFAILED);
        PKIX_CHECK(PKIX_PL_String_GetEncoded
                    (stateString,
                    PKIX_ESCASCII,
                    (void **)&stateAscii,
                    &length,
                    plContext),
                    PKIX_STRINGGETENCODEDFAILED);
        PKIX_DEBUG_ARG("%s\n", stateAscii);

        PKIX_FREE(stateAscii);
        PKIX_DECREF(stateString);
#endif

cleanup:

        PKIX_DECREF(nominees);

        PKIX_RETURN(CERTCHAINCHECKER);
}
















static PKIX_Error *
pkix_PolicyChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticals,  
        void **pNBIOContext,
        void *plContext)
{
        PKIX_UInt32 numPolicies = 0;
        PKIX_UInt32 polX = 0;
        PKIX_Boolean result = PKIX_FALSE;
        PKIX_Int32 inhibitMappingSkipCerts = 0;
        PKIX_Int32 explicitPolicySkipCerts = 0;
        PKIX_Int32 inhibitAnyPolicySkipCerts = 0;
        PKIX_Boolean shouldBePruned = PKIX_FALSE;
        PKIX_Boolean isSelfIssued = PKIX_FALSE;
        PKIX_Boolean certPoliciesIncludeAny = PKIX_FALSE;
        PKIX_Boolean doAnyPolicyProcessing = PKIX_FALSE;

        PKIX_PolicyCheckerState *state = NULL;
        PKIX_List *certPolicyInfos = NULL; 
        PKIX_PL_CertPolicyInfo *policy = NULL;
        PKIX_PL_OID *policyOID = NULL;
        PKIX_List *qualsOfAny = NULL; 
        PKIX_List *policyQualifiers = NULL; 
        PKIX_List *policyMaps = NULL; 
        PKIX_List *mappedPolicies = NULL; 
        PKIX_Error *subroutineErr = NULL;
#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        PKIX_PL_String *stateString = NULL;
        char *stateAscii = NULL;
        PKIX_PL_String *certString = NULL;
        char *certAscii = NULL;
        PKIX_UInt32 length;
#endif

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_Check");
        PKIX_NULLCHECK_FOUR(checker, cert, unresolvedCriticals, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        PKIX_NULLCHECK_TWO(state, state->certPoliciesExtension);

#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        PKIX_CHECK(PKIX_PL_Object_ToString
                ((PKIX_PL_Object*)state, &stateString, plContext),
                PKIX_OBJECTTOSTRINGFAILED);
        PKIX_CHECK(PKIX_PL_String_GetEncoded
                    (stateString,
                    PKIX_ESCASCII,
                    (void **)&stateAscii,
                    &length,
                    plContext),
                    PKIX_STRINGGETENCODEDFAILED);
        PKIX_DEBUG_ARG("On entry %s\n", stateAscii);
        PKIX_FREE(stateAscii);
        PKIX_DECREF(stateString);
#endif

        






        if (state->certsProcessed != (state->numCerts - 1)) {
                PKIX_CHECK(PKIX_PL_Cert_GetPolicyMappings
                        (cert, &policyMaps, plContext),
                        PKIX_CERTGETPOLICYMAPPINGSFAILED);
        }

        if (policyMaps) {

                PKIX_CHECK(pkix_PolicyChecker_MapContains
                        (policyMaps, state->anyPolicyOID, &result, plContext),
                        PKIX_POLICYCHECKERMAPCONTAINSFAILED);

                if (result) {
                        PKIX_ERROR(PKIX_INVALIDPOLICYMAPPINGINCLUDESANYPOLICY);
                }

                PKIX_CHECK(pkix_PolicyChecker_MapGetMappedPolicies
                        (policyMaps, &mappedPolicies, plContext),
                        PKIX_POLICYCHECKERMAPGETMAPPEDPOLICIESFAILED);

                PKIX_DECREF(state->mappedPolicyOIDs);
                PKIX_INCREF(mappedPolicies);
                state->mappedPolicyOIDs = mappedPolicies;
        }

        
        if (state->validPolicyTree) {

            PKIX_CHECK(PKIX_PL_Cert_GetPolicyInformation
                (cert, &certPolicyInfos, plContext),
                PKIX_CERTGETPOLICYINFORMATIONFAILED);

            if (certPolicyInfos) {
                PKIX_CHECK(PKIX_List_GetLength
                        (certPolicyInfos, &numPolicies, plContext),
                        PKIX_LISTGETLENGTHFAILED);
            }

            if (numPolicies > 0) {

                PKIX_CHECK(PKIX_PL_Cert_AreCertPoliciesCritical
                        (cert, &(state->certPoliciesCritical), plContext),
                        PKIX_CERTARECERTPOLICIESCRITICALFAILED);

                
                for (polX = 0; polX < numPolicies; polX++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (certPolicyInfos,
                        polX,
                        (PKIX_PL_Object **)&policy,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK(PKIX_PL_CertPolicyInfo_GetPolicyId
                        (policy, &policyOID, plContext),
                        PKIX_CERTPOLICYINFOGETPOLICYIDFAILED);

                    PKIX_CHECK(PKIX_PL_CertPolicyInfo_GetPolQualifiers
                        (policy, &policyQualifiers, plContext),
                        PKIX_CERTPOLICYINFOGETPOLQUALIFIERSFAILED);

                    PKIX_EQUALS
                        (state->anyPolicyOID,
                        policyOID,
                        &result,
                        plContext,
                        PKIX_OIDEQUALFAILED);

                    if (result == PKIX_FALSE) {

                        
                        subroutineErr = pkix_PolicyChecker_CheckPolicy
                                (policyOID,
                                policyQualifiers,
                                cert,
                                policyMaps,
                                state,
                                plContext);
                        if (subroutineErr) {
                                goto subrErrorCleanup;
                        }

                    } else {
                        



                        PKIX_DECREF(qualsOfAny);
                        PKIX_INCREF(policyQualifiers);
                        qualsOfAny = policyQualifiers;
                        certPoliciesIncludeAny = PKIX_TRUE;
                    }
                    PKIX_DECREF(policy);
                    PKIX_DECREF(policyOID);
                    PKIX_DECREF(policyQualifiers);
                }

                
                if (certPoliciesIncludeAny == PKIX_TRUE) {
                        if (state->inhibitAnyPolicy > 0) {
                                doAnyPolicyProcessing = PKIX_TRUE;
                        } else {
                            
                            if (((state->certsProcessed) + 1) <
                                (state->numCerts)) {

                                PKIX_CHECK(pkix_IsCertSelfIssued
                                        (cert,
                                        &doAnyPolicyProcessing,
                                        plContext),
                                        PKIX_ISCERTSELFISSUEDFAILED);
                            }
                        }
                        if (doAnyPolicyProcessing) {
                            subroutineErr = pkix_PolicyChecker_CheckAny
                                (state->validPolicyTree,
                                qualsOfAny,
                                policyMaps,
                                state,
                                plContext);
                            if (subroutineErr) {
                                goto subrErrorCleanup;
                            }
                        }
                }

                
                if (state->validPolicyTree) {
                        subroutineErr = pkix_PolicyNode_Prune
                                (state->validPolicyTree,
                                state->certsProcessed + 1,
                                &shouldBePruned,
                                plContext);
                        if (subroutineErr) {
                                goto subrErrorCleanup;
                        }
                        if (shouldBePruned) {
                                PKIX_DECREF(state->validPolicyTree);
                                PKIX_DECREF(state->anyPolicyNodeAtBottom);
                        }
                }

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)state, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);

            } else {
                
                PKIX_DECREF(state->validPolicyTree);
                PKIX_DECREF(state->anyPolicyNodeAtBottom);
                PKIX_DECREF(state->newAnyPolicyNode);

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)state, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);
            }
        }

        
        if ((0 == state->explicitPolicy) && (!state->validPolicyTree)) {
                PKIX_ERROR(PKIX_CERTCHAINFAILSCERTIFICATEPOLICYVALIDATION);
        }

        



        PKIX_CHECK(pkix_List_Remove
                (unresolvedCriticals,
                (PKIX_PL_Object *)state->certPoliciesExtension,
                plContext),
                PKIX_LISTREMOVEFAILED);

        PKIX_CHECK(pkix_List_Remove
                (unresolvedCriticals,
                (PKIX_PL_Object *)state->policyMappingsExtension,
                plContext),
                PKIX_LISTREMOVEFAILED);

        PKIX_CHECK(pkix_List_Remove
                (unresolvedCriticals,
                (PKIX_PL_Object *)state->policyConstraintsExtension,
                plContext),
                PKIX_LISTREMOVEFAILED);

        PKIX_CHECK(pkix_List_Remove
                (unresolvedCriticals,
                (PKIX_PL_Object *)state->inhibitAnyPolicyExtension,
                plContext),
                PKIX_LISTREMOVEFAILED);

        state->certsProcessed++;

        
        if (state->certsProcessed != state->numCerts) {

                if (policyMaps) {
                        subroutineErr = pkix_PolicyChecker_PolicyMapProcessing
                                (policyMaps,
                                certPoliciesIncludeAny,
                                qualsOfAny,
                                state,
                                plContext);
                        if (subroutineErr) {
                                goto subrErrorCleanup;
                        }
                }

                
                PKIX_DECREF(state->anyPolicyNodeAtBottom);
                state->anyPolicyNodeAtBottom = state->newAnyPolicyNode;
                state->newAnyPolicyNode = NULL;

                
                PKIX_CHECK(pkix_IsCertSelfIssued
                        (cert, &isSelfIssued, plContext),
                        PKIX_ISCERTSELFISSUEDFAILED);

                if (!isSelfIssued) {
                        if (state->explicitPolicy > 0) {
                            state->explicitPolicy--;
                        }
                        if (state->policyMapping > 0) {
                            state->policyMapping--;
                        }
                        if (state->inhibitAnyPolicy > 0) {
                            state->inhibitAnyPolicy--;
                        }
                }

                
                PKIX_CHECK(PKIX_PL_Cert_GetRequireExplicitPolicy
                        (cert, &explicitPolicySkipCerts, plContext),
                        PKIX_CERTGETREQUIREEXPLICITPOLICYFAILED);

                if (explicitPolicySkipCerts != -1) {
                        if (((PKIX_UInt32)explicitPolicySkipCerts) <
                            (state->explicitPolicy)) {
                                state->explicitPolicy =
                                   ((PKIX_UInt32) explicitPolicySkipCerts);
                        }
                }

                PKIX_CHECK(PKIX_PL_Cert_GetPolicyMappingInhibited
                        (cert, &inhibitMappingSkipCerts, plContext),
                        PKIX_CERTGETPOLICYMAPPINGINHIBITEDFAILED);

                if (inhibitMappingSkipCerts != -1) {
                        if (((PKIX_UInt32)inhibitMappingSkipCerts) <
                            (state->policyMapping)) {
                                state->policyMapping =
                                    ((PKIX_UInt32)inhibitMappingSkipCerts);
                        }
                }

                PKIX_CHECK(PKIX_PL_Cert_GetInhibitAnyPolicy
                        (cert, &inhibitAnyPolicySkipCerts, plContext),
                        PKIX_CERTGETINHIBITANYPOLICYFAILED);

                if (inhibitAnyPolicySkipCerts != -1) {
                        if (((PKIX_UInt32)inhibitAnyPolicySkipCerts) <
                            (state->inhibitAnyPolicy)) {
                                state->inhibitAnyPolicy =
                                    ((PKIX_UInt32)inhibitAnyPolicySkipCerts);
                        }
                }

                PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                        ((PKIX_PL_Object *)state, plContext),
                        PKIX_OBJECTINVALIDATECACHEFAILED);

        } else { 

                
                subroutineErr = pkix_PolicyChecker_WrapUpProcessing
                        (cert, state, plContext);
                if (subroutineErr) {
                        goto subrErrorCleanup;
                }

                if ((0 == state->explicitPolicy) && (!state->validPolicyTree)) {
                    PKIX_ERROR(PKIX_CERTCHAINFAILSCERTIFICATEPOLICYVALIDATION);
                }

                PKIX_DECREF(state->anyPolicyNodeAtBottom);
                PKIX_DECREF(state->newAnyPolicyNode);
        }


        if (subroutineErr) {

subrErrorCleanup:
                
                pkixErrorClass = subroutineErr->errClass;
                if (pkixErrorClass == PKIX_FATAL_ERROR) {
                    pkixErrorResult = subroutineErr;
                    subroutineErr = NULL;
                    goto cleanup;
                }
                



                PKIX_DECREF(state->validPolicyTree);
                PKIX_DECREF(state->anyPolicyNodeAtBottom);
                PKIX_DECREF(state->newAnyPolicyNode);
                if (state->explicitPolicy == 0) {
                    PKIX_ERROR
                        (PKIX_CERTCHAINFAILSCERTIFICATEPOLICYVALIDATION);
                }
        }

        
        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                (checker, (PKIX_PL_Object *)state, plContext),
                PKIX_CERTCHAINCHECKERSETCERTCHAINCHECKERSTATEFAILED);

cleanup:

#if PKIX_CERTPOLICYCHECKERSTATEDEBUG
        if (cert) {
                PKIX_CHECK(PKIX_PL_Object_ToString
                        ((PKIX_PL_Object*)cert, &certString, plContext),
                        PKIX_OBJECTTOSTRINGFAILED);
                PKIX_CHECK(PKIX_PL_String_GetEncoded
                            (certString,
                            PKIX_ESCASCII,
                            (void **)&certAscii,
                            &length,
                            plContext),
                            PKIX_STRINGGETENCODEDFAILED);
                PKIX_DEBUG_ARG("Cert was %s\n", certAscii);
                PKIX_FREE(certAscii);
                PKIX_DECREF(certString);
        }
        if (state) {
                PKIX_CHECK(PKIX_PL_Object_ToString
                        ((PKIX_PL_Object*)state, &stateString, plContext),
                        PKIX_OBJECTTOSTRINGFAILED);
                PKIX_CHECK(PKIX_PL_String_GetEncoded
                            (stateString,
                            PKIX_ESCASCII,
                            (void **)&stateAscii,
                            &length,
                            plContext),
                            PKIX_STRINGGETENCODEDFAILED);
                PKIX_DEBUG_ARG("On exit %s\n", stateAscii);
                PKIX_FREE(stateAscii);
                PKIX_DECREF(stateString);
        }
#endif

        PKIX_DECREF(state);
        PKIX_DECREF(certPolicyInfos);
        PKIX_DECREF(policy);
        PKIX_DECREF(qualsOfAny);
        PKIX_DECREF(policyQualifiers);
        PKIX_DECREF(policyOID);
        PKIX_DECREF(subroutineErr);
        PKIX_DECREF(policyMaps);
        PKIX_DECREF(mappedPolicies);

        PKIX_RETURN(CERTCHAINCHECKER);
}








































PKIX_Error *
pkix_PolicyChecker_Initialize(
        PKIX_List *initialPolicies,
        PKIX_Boolean policyQualifiersRejected,
        PKIX_Boolean initialPolicyMappingInhibit,
        PKIX_Boolean initialExplicitPolicy,
        PKIX_Boolean initialAnyPolicyInhibit,
        PKIX_UInt32 numCerts,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        PKIX_PolicyCheckerState *polCheckerState = NULL;
        PKIX_List *policyExtensions = NULL;     
        PKIX_ENTER(CERTCHAINCHECKER, "pkix_PolicyChecker_Initialize");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(pkix_PolicyCheckerState_Create
                (initialPolicies,
                policyQualifiersRejected,
                initialPolicyMappingInhibit,
                initialExplicitPolicy,
                initialAnyPolicyInhibit,
                numCerts,
                &polCheckerState,
                plContext),
                PKIX_POLICYCHECKERSTATECREATEFAILED);

        
        PKIX_CHECK(pkix_PolicyChecker_MakeSingleton
                ((PKIX_PL_Object *)(polCheckerState->certPoliciesExtension),
                PKIX_TRUE,
                &policyExtensions,
                plContext),
                PKIX_POLICYCHECKERMAKESINGLETONFAILED);

        PKIX_CHECK(PKIX_CertChainChecker_Create
                (pkix_PolicyChecker_Check,
                PKIX_FALSE,     
                PKIX_FALSE,
                policyExtensions,
                (PKIX_PL_Object *)polCheckerState,
                pChecker,
                plContext),
                PKIX_CERTCHAINCHECKERCREATEFAILED);

cleanup:
        PKIX_DECREF(polCheckerState);
        PKIX_DECREF(policyExtensions);
        PKIX_RETURN(CERTCHAINCHECKER);

}
