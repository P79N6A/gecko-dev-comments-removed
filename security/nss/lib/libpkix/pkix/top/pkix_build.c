













































#include "pkix_build.h"

extern PRLogModuleInfo *pkixLog;






static SECOidTag buildCheckedCritExtOIDs[] = {
        PKIX_CERTKEYUSAGE_OID,
        PKIX_CERTSUBJALTNAME_OID,
        PKIX_BASICCONSTRAINTS_OID,
        PKIX_NAMECONSTRAINTS_OID,
        PKIX_EXTENDEDKEYUSAGE_OID,
        PKIX_NSCERTTYPE_OID,
        PKIX_UNKNOWN_OID
};







static PKIX_Error *
pkix_ForwardBuilderState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_ForwardBuilderState *state = NULL;

        PKIX_ENTER(FORWARDBUILDERSTATE, "pkix_ForwardBuilderState_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_FORWARDBUILDERSTATE_TYPE, plContext),
                PKIX_OBJECTNOTFORWARDBUILDERSTATE);

        state = (PKIX_ForwardBuilderState *)object;

        state->status = BUILD_INITIAL;
        state->traversedCACerts = 0;
        state->certStoreIndex = 0;
        state->numCerts = 0;
        state->numAias = 0;
        state->certIndex = 0;
        state->aiaIndex = 0;
        state->certCheckedIndex = 0;
        state->checkerIndex = 0;
        state->hintCertIndex = 0;
        state->numFanout = 0;
        state->numDepth = 0;
        state->reasonCode = 0;
        state->revCheckDelayed = PKIX_FALSE;
        state->canBeCached = PKIX_FALSE;
        state->useOnlyLocal = PKIX_FALSE;
        state->revChecking = PKIX_FALSE;
        state->usingHintCerts = PKIX_FALSE;
        state->certLoopingDetected = PKIX_FALSE;
        PKIX_DECREF(state->validityDate);
        PKIX_DECREF(state->prevCert);
        PKIX_DECREF(state->candidateCert);
        PKIX_DECREF(state->traversedSubjNames);
        PKIX_DECREF(state->trustChain);
        PKIX_DECREF(state->aia);
        PKIX_DECREF(state->candidateCerts);
        PKIX_DECREF(state->reversedCertChain);
        PKIX_DECREF(state->checkedCritExtOIDs);
        PKIX_DECREF(state->checkerChain);
        PKIX_DECREF(state->certSel);
        PKIX_DECREF(state->verifyNode);
        PKIX_DECREF(state->client);

        



        if (state->parentState == NULL) {
                state->buildConstants.numAnchors = 0;
                state->buildConstants.numCertStores = 0;
                state->buildConstants.numHintCerts = 0;
                state->buildConstants.procParams = 0;
                PKIX_DECREF(state->buildConstants.testDate);
                PKIX_DECREF(state->buildConstants.timeLimit);
                PKIX_DECREF(state->buildConstants.targetCert);
                PKIX_DECREF(state->buildConstants.targetPubKey);
                PKIX_DECREF(state->buildConstants.certStores);
                PKIX_DECREF(state->buildConstants.anchors);
                PKIX_DECREF(state->buildConstants.userCheckers);
                PKIX_DECREF(state->buildConstants.hintCerts);
                PKIX_DECREF(state->buildConstants.revChecker);
                PKIX_DECREF(state->buildConstants.aiaMgr);
        } else {
                PKIX_DECREF(state->parentState);
        }

cleanup:

        PKIX_RETURN(FORWARDBUILDERSTATE);
}










































static PKIX_Error *
pkix_ForwardBuilderState_Create(
        PKIX_Int32 traversedCACerts,
        PKIX_UInt32 numFanout,
        PKIX_UInt32 numDepth,
        PKIX_Boolean revCheckDelayed,
        PKIX_Boolean canBeCached,
        PKIX_PL_Date *validityDate,
        PKIX_PL_Cert *prevCert,
        PKIX_List *traversedSubjNames,
        PKIX_List *trustChain,
        PKIX_ForwardBuilderState *parentState,
        PKIX_ForwardBuilderState **pState,
        void *plContext)
{
        PKIX_ForwardBuilderState *state = NULL;

        PKIX_ENTER(FORWARDBUILDERSTATE, "pkix_ForwardBuilderState_Create");
        PKIX_NULLCHECK_FOUR(prevCert, traversedSubjNames, pState, trustChain);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_FORWARDBUILDERSTATE_TYPE,
                sizeof (PKIX_ForwardBuilderState),
                (PKIX_PL_Object **)&state,
                plContext),
                PKIX_COULDNOTCREATEFORWARDBUILDERSTATEOBJECT);

        state->status = BUILD_INITIAL;
        state->traversedCACerts = traversedCACerts;
        state->certStoreIndex = 0;
        state->numCerts = 0;
        state->numAias = 0;
        state->certIndex = 0;
        state->aiaIndex = 0;
        state->certCheckedIndex = 0;
        state->checkerIndex = 0;
        state->hintCertIndex = 0;
        state->numFanout = numFanout;
        state->numDepth = numDepth;
        state->reasonCode = 0;
        state->revChecking = numDepth;
        state->revCheckDelayed = revCheckDelayed;
        state->canBeCached = canBeCached;
        state->useOnlyLocal = PKIX_TRUE;
        state->revChecking = PKIX_FALSE;
        state->usingHintCerts = PKIX_FALSE;
        state->certLoopingDetected = PKIX_FALSE;

        PKIX_INCREF(validityDate);
        state->validityDate = validityDate;

        PKIX_INCREF(prevCert);
        state->prevCert = prevCert;

        state->candidateCert = NULL;

        PKIX_INCREF(traversedSubjNames);
        state->traversedSubjNames = traversedSubjNames;

        PKIX_INCREF(trustChain);
        state->trustChain = trustChain;

        state->aia = NULL;
        state->candidateCerts = NULL;
        state->reversedCertChain = NULL;
        state->checkedCritExtOIDs = NULL;
        state->checkerChain = NULL;
        state->certSel = NULL;
        state->verifyNode = NULL;
        state->client = NULL;

        PKIX_INCREF(parentState);
        state->parentState = parentState;

        if (parentState != NULL) {
                state->buildConstants.numAnchors =
                         parentState->buildConstants.numAnchors;
                state->buildConstants.numCertStores = 
                        parentState->buildConstants.numCertStores; 
                state->buildConstants.numHintCerts = 
                        parentState->buildConstants.numHintCerts; 
                state->buildConstants.maxFanout =
                        parentState->buildConstants.maxFanout;
                state->buildConstants.maxDepth =
                        parentState->buildConstants.maxDepth;
                state->buildConstants.maxTime =
                        parentState->buildConstants.maxTime;
                state->buildConstants.procParams = 
                        parentState->buildConstants.procParams; 
                state->buildConstants.testDate =
                        parentState->buildConstants.testDate;
                state->buildConstants.timeLimit =
                        parentState->buildConstants.timeLimit;
                state->buildConstants.targetCert =
                        parentState->buildConstants.targetCert;
                state->buildConstants.targetPubKey =
                        parentState->buildConstants.targetPubKey;
                state->buildConstants.certStores =
                        parentState->buildConstants.certStores;
                state->buildConstants.anchors =
                        parentState->buildConstants.anchors;
                state->buildConstants.userCheckers =
                        parentState->buildConstants.userCheckers;
                state->buildConstants.hintCerts =
                        parentState->buildConstants.hintCerts;
                state->buildConstants.revChecker =
                        parentState->buildConstants.revChecker;
                state->buildConstants.aiaMgr =
                        parentState->buildConstants.aiaMgr;
        }

        *pState = state;
        state = NULL;
cleanup:
        
        PKIX_DECREF(state);

        PKIX_RETURN(FORWARDBUILDERSTATE);
}






















static PKIX_Error *
pkix_Build_GetResourceLimits(
        BuildConstants *buildConstants,
        void *plContext)
{
        PKIX_ResourceLimits *resourceLimits = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_GetResourceLimits");
        PKIX_NULLCHECK_ONE(buildConstants);

        PKIX_CHECK(PKIX_ProcessingParams_GetResourceLimits
                (buildConstants->procParams, &resourceLimits, plContext),
                PKIX_PROCESSINGPARAMSGETRESOURCELIMITSFAILED);

        buildConstants->maxFanout = 0;
        buildConstants->maxDepth = 0;
        buildConstants->maxTime = 0;

        if (resourceLimits) {

                PKIX_CHECK(PKIX_ResourceLimits_GetMaxFanout
                        (resourceLimits, &buildConstants->maxFanout, plContext),
                        PKIX_RESOURCELIMITSGETMAXFANOUTFAILED);

                PKIX_CHECK(PKIX_ResourceLimits_GetMaxDepth
                        (resourceLimits, &buildConstants->maxDepth, plContext),
                        PKIX_RESOURCELIMITSGETMAXDEPTHFAILED);

                PKIX_CHECK(PKIX_ResourceLimits_GetMaxTime
                        (resourceLimits, &buildConstants->maxTime, plContext),
                        PKIX_RESOURCELIMITSGETMAXTIMEFAILED);
        }

cleanup:

        PKIX_DECREF(resourceLimits);

        PKIX_RETURN(BUILD);
}





static PKIX_Error *
pkix_ForwardBuilderState_ToString
        (PKIX_PL_Object *object,
        PKIX_PL_String **pString,
        void *plContext)
{
        PKIX_ForwardBuilderState *state = NULL;
        PKIX_PL_String *formatString = NULL;
        PKIX_PL_String *resultString = NULL;
        PKIX_PL_String *buildStatusString = NULL;
        PKIX_PL_String *validityDateString = NULL;
        PKIX_PL_String *prevCertString = NULL;
        PKIX_PL_String *candidateCertString = NULL;
        PKIX_PL_String *traversedSubjNamesString = NULL;
        PKIX_PL_String *trustChainString = NULL;
        PKIX_PL_String *candidateCertsString = NULL;
        PKIX_PL_String *certSelString = NULL;
        PKIX_PL_String *verifyNodeString = NULL;
        PKIX_PL_String *parentStateString = NULL;
        char *asciiFormat = "\n"
                "\t{buildStatus: \t%s\n"
                "\ttraversedCACerts: \t%d\n"
                "\tcertStoreIndex: \t%d\n"
                "\tnumCerts: \t%d\n"
                "\tnumAias: \t%d\n"
                "\tcertIndex: \t%d\n"
                "\taiaIndex: \t%d\n"
                "\tnumFanout: \t%d\n"
                "\tnumDepth:  \t%d\n"
                "\treasonCode:  \t%d\n"
                "\trevCheckDelayed: \t%d\n"
                "\tcanBeCached: \t%d\n"
                "\tuseOnlyLocal: \t%d\n"
                "\trevChecking: \t%d\n"
                "\tvalidityDate: \t%s\n"
                "\tprevCert: \t%s\n"
                "\tcandidateCert: \t%s\n"
                "\ttraversedSubjNames: \t%s\n"
                "\ttrustChain: \t%s\n"
                "\tcandidateCerts: \t%s\n"
                "\tcertSel: \t%s\n"
                "\tverifyNode: \t%s\n"
                "\tparentState: \t%s}\n";
        char *asciiStatus = NULL;

        PKIX_ENTER(FORWARDBUILDERSTATE, "pkix_ForwardBuilderState_ToString");
        PKIX_NULLCHECK_TWO(object, pString);

        PKIX_CHECK(pkix_CheckType
                (object, PKIX_FORWARDBUILDERSTATE_TYPE, plContext),
                PKIX_OBJECTNOTFORWARDBUILDERSTATE);

        state = (PKIX_ForwardBuilderState *)object;

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, asciiFormat, 0, &formatString, plContext),
                PKIX_STRINGCREATEFAILED);

        switch (state->status) {
            case BUILD_SHORTCUTPENDING: asciiStatus = "BUILD_SHORTCUTPENDING";
                                        break;
            case BUILD_INITIAL:         asciiStatus = "BUILD_INITIAL";
                                        break;
            case BUILD_TRYAIA:          asciiStatus = "BUILD_TRYAIA";
                                        break;
            case BUILD_AIAPENDING:      asciiStatus = "BUILD_AIAPENDING";
                                        break;
            case BUILD_COLLECTINGCERTS: asciiStatus = "BUILD_COLLECTINGCERTS";
                                        break;
            case BUILD_GATHERPENDING:   asciiStatus = "BUILD_GATHERPENDING";
                                        break;
            case BUILD_CERTVALIDATING:  asciiStatus = "BUILD_CERTVALIDATING";
                                        break;
            case BUILD_ABANDONNODE:     asciiStatus = "BUILD_ABANDONNODE";
                                        break;
            case BUILD_CRLPREP:         asciiStatus = "BUILD_CRLPREP";
                                        break;
            case BUILD_CRL1:            asciiStatus = "BUILD_CRL1";
                                        break;
            case BUILD_DATEPREP:        asciiStatus = "BUILD_DATEPREP";
                                        break;
            case BUILD_CHECKTRUSTED:    asciiStatus = "BUILD_CHECKTRUSTED";
                                        break;
            case BUILD_CHECKTRUSTED2:   asciiStatus = "BUILD_CHECKTRUSTED2";
                                        break;
            case BUILD_ADDTOCHAIN:      asciiStatus = "BUILD_ADDTOCHAIN";
                                        break;
            case BUILD_CRL2:            asciiStatus = "BUILD_CRL2";
                                        break;
            case BUILD_VALCHAIN:        asciiStatus = "BUILD_VALCHAIN";
                                        break;
            case BUILD_VALCHAIN2:       asciiStatus = "BUILD_VALCHAIN2";
                                        break;
            case BUILD_EXTENDCHAIN:     asciiStatus = "BUILD_EXTENDCHAIN";
                                        break;
            case BUILD_GETNEXTCERT:     asciiStatus = "BUILD_GETNEXTCERT";
                                        break;
            default:                    asciiStatus = "INVALID STATUS";
                                        break;
        }

        PKIX_CHECK(PKIX_PL_String_Create
                (PKIX_ESCASCII, asciiStatus, 0, &buildStatusString, plContext),
                PKIX_STRINGCREATEFAILED);

        PKIX_TOSTRING
               (state->validityDate, &validityDateString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
               (state->prevCert, &prevCertString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->candidateCert, &candidateCertString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->traversedSubjNames,
                &traversedSubjNamesString,
                plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->trustChain, &trustChainString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->candidateCerts, &candidateCertsString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->certSel, &certSelString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->verifyNode, &verifyNodeString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_TOSTRING
                (state->parentState, &parentStateString, plContext,
                PKIX_OBJECTTOSTRINGFAILED);

        PKIX_CHECK(PKIX_PL_Sprintf
                (&resultString,
                plContext,
                formatString,
                buildStatusString,
                (PKIX_Int32)state->traversedCACerts,
                (PKIX_UInt32)state->certStoreIndex,
                (PKIX_UInt32)state->numCerts,
                (PKIX_UInt32)state->numAias,
                (PKIX_UInt32)state->certIndex,
                (PKIX_UInt32)state->aiaIndex,
                (PKIX_UInt32)state->numFanout,
                (PKIX_UInt32)state->numDepth,
                (PKIX_UInt32)state->reasonCode,
                state->revCheckDelayed,
                state->canBeCached,
                state->useOnlyLocal,
                state->revChecking,
                validityDateString,
                prevCertString,
                candidateCertString,
                traversedSubjNamesString,
                trustChainString,
                candidateCertsString,
                certSelString,
                verifyNodeString,
                parentStateString),
                PKIX_SPRINTFFAILED);

        *pString = resultString;

cleanup:
        PKIX_DECREF(formatString);
        PKIX_DECREF(buildStatusString);
        PKIX_DECREF(validityDateString);
        PKIX_DECREF(prevCertString);
        PKIX_DECREF(candidateCertString);
        PKIX_DECREF(traversedSubjNamesString);
        PKIX_DECREF(trustChainString);
        PKIX_DECREF(candidateCertsString);
        PKIX_DECREF(certSelString);
        PKIX_DECREF(verifyNodeString);
        PKIX_DECREF(parentStateString);

        PKIX_RETURN(FORWARDBUILDERSTATE);

}















PKIX_Error *
pkix_ForwardBuilderState_RegisterSelf(void *plContext)
{

        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(FORWARDBUILDERSTATE,
                    "pkix_ForwardBuilderState_RegisterSelf");

        entry.description = "ForwardBuilderState";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_ForwardBuilderState);
        entry.destructor = pkix_ForwardBuilderState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = pkix_ForwardBuilderState_ToString;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_FORWARDBUILDERSTATE_TYPE] = entry;

        PKIX_RETURN(FORWARDBUILDERSTATE);
}

#if PKIX_FORWARDBUILDERSTATEDEBUG













PKIX_Error *
pkix_ForwardBuilderState_DumpState(
        PKIX_ForwardBuilderState *state,
        void *plContext)
{
        PKIX_PL_String *stateString = NULL;
        char *stateAscii = NULL;
        PKIX_UInt32 length;

        PKIX_ENTER(FORWARDBUILDERSTATE,"pkix_ForwardBuilderState_DumpState");
        PKIX_NULLCHECK_ONE(state);

        PKIX_CHECK(PKIX_PL_Object_InvalidateCache
                ((PKIX_PL_Object *)state, plContext),
                PKIX_OBJECTINVALIDATECACHEFAILED);

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

        PKIX_DEBUG_ARG("In Phase 1: state = %s\n", stateAscii);

        PKIX_FREE(stateAscii);
        PKIX_DECREF(stateString);

cleanup:
        PKIX_RETURN(FORWARDBUILDERSTATE);
}
#endif
























static PKIX_Error*
pkix_ForwardBuilderState_IsIOPending(
        PKIX_ForwardBuilderState *state,
        PKIX_Boolean *pPending,
        void *plContext)
{
        PKIX_ENTER(FORWARDBUILDERSTATE, "pkix_ForwardBuilderState_IsIOPending");
        PKIX_NULLCHECK_TWO(state, pPending);

        if ((state->status == BUILD_GATHERPENDING) ||
            (state->status == BUILD_CRL1) ||
            (state->status == BUILD_CRL2) ||
            (state->status == BUILD_CHECKTRUSTED2) ||
            (state->status == BUILD_VALCHAIN2) ||
            (state->status == BUILD_AIAPENDING)) {
                *pPending = PKIX_TRUE;
        } else {
                *pPending = PKIX_FALSE;
        }

        PKIX_RETURN(FORWARDBUILDERSTATE);
}






























static PKIX_Error *
pkix_Build_SortCertComparator(
        PKIX_PL_Object *obj1,
        PKIX_PL_Object *obj2,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_Date *date1 = NULL;
        PKIX_PL_Date *date2 = NULL;
        PKIX_Boolean result = PKIX_FALSE;

        PKIX_ENTER(BUILD, "pkix_Build_SortCertComparator");
        PKIX_NULLCHECK_THREE(obj1, obj2, pResult);

        










        PKIX_CHECK(pkix_CheckType(obj1, PKIX_CERT_TYPE, plContext),
                    PKIX_OBJECTNOTCERT);
        PKIX_CHECK(pkix_CheckType(obj2, PKIX_CERT_TYPE, plContext),
                    PKIX_OBJECTNOTCERT);

        PKIX_CHECK(PKIX_PL_Cert_GetValidityNotAfter
                ((PKIX_PL_Cert *)obj1, &date1, plContext),
                PKIX_CERTGETVALIDITYNOTAFTERFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetValidityNotAfter
                ((PKIX_PL_Cert *)obj2, &date2, plContext),
                PKIX_CERTGETVALIDITYNOTAFTERFAILED);
        
        PKIX_CHECK(PKIX_PL_Object_Compare
                ((PKIX_PL_Object *)date1,
                (PKIX_PL_Object *)date2,
                &result,
                plContext),
                PKIX_OBJECTCOMPARATORFAILED);

        *pResult = !result;

cleanup:

        PKIX_DECREF(date1);
        PKIX_DECREF(date2);

        PKIX_RETURN(BUILD);
}


#define ERROR_CHECK(errCode) \
    if (pkixErrorResult) { \
        if (pkixLog) { \
            PR_LOG(pkixLog, PR_LOG_DEBUG, ("====> ERROR_CHECK code %s\n", #errCode)); \
        } \
        pkixTempErrorReceived = PKIX_TRUE; \
        pkixErrorClass = pkixErrorResult->errClass; \
        if (pkixErrorClass == PKIX_FATAL_ERROR) { \
            goto cleanup; \
        } \
        if (verifyNode) { \
            PKIX_DECREF(verifyNode->error); \
            PKIX_INCREF(pkixErrorResult); \
            verifyNode->error = pkixErrorResult; \
        } \
        pkixErrorCode = errCode; \
        goto cleanup; \
    }













































static PKIX_Error *
pkix_Build_VerifyCertificate(
        PKIX_ForwardBuilderState *state,
        PKIX_List *userCheckers,
        PKIX_Boolean revocationChecking,
        PKIX_Boolean *pTrusted,
        PKIX_Boolean *pNeedsCRLChecking,
        PKIX_VerifyNode *verifyNode,
        void *plContext)
{
        PKIX_UInt32 numUserCheckers = 0;
        PKIX_UInt32 i = 0;
        PKIX_Boolean loopFound = PKIX_FALSE;
        PKIX_Boolean supportForwardChecking = PKIX_FALSE;
        PKIX_Boolean trusted = PKIX_FALSE;
        PKIX_PL_Cert *candidateCert = NULL;
        PKIX_PL_PublicKey *candidatePubKey = NULL;
        PKIX_CertChainChecker *userChecker = NULL;
        PKIX_CertChainChecker_CheckCallback checkerCheck = NULL;
        PKIX_Boolean trustOnlyUserAnchors = PKIX_FALSE;
        void *nbioContext = NULL;
        
        PKIX_ENTER(BUILD, "pkix_Build_VerifyCertificate");
        PKIX_NULLCHECK_THREE(state, pTrusted, pNeedsCRLChecking);
        PKIX_NULLCHECK_THREE
                (state->candidateCerts, state->prevCert, state->trustChain);

        *pNeedsCRLChecking = PKIX_FALSE;

        PKIX_INCREF(state->candidateCert);
        candidateCert = state->candidateCert;

        

        if (state->buildConstants.numAnchors) {
            trustOnlyUserAnchors = PKIX_TRUE;
        }

        PKIX_CHECK(
            PKIX_PL_Cert_IsCertTrusted(candidateCert,
                                       trustOnlyUserAnchors,
                                       &trusted, plContext),
            PKIX_CERTISCERTTRUSTEDFAILED);

        *pTrusted = trusted;

        
        PKIX_CHECK(pkix_List_Contains
                (state->trustChain,
                (PKIX_PL_Object *)candidateCert,
                &loopFound,
                plContext),
                PKIX_LISTCONTAINSFAILED);

        if (loopFound) {
                if (verifyNode != NULL) {
                        PKIX_Error *verifyError = NULL;
                        PKIX_ERROR_CREATE
                                (BUILD,
                                PKIX_LOOPDISCOVEREDDUPCERTSNOTALLOWED,
                                verifyError);
                        PKIX_DECREF(verifyNode->error);
                        verifyNode->error = verifyError;
                }
                

                if (!trusted) {
                        PKIX_ERROR(PKIX_LOOPDISCOVEREDDUPCERTSNOTALLOWED);
                }
                state->certLoopingDetected = PKIX_TRUE;
        }

        if (userCheckers != NULL) {

                PKIX_CHECK(PKIX_List_GetLength
                    (userCheckers, &numUserCheckers, plContext),
                    PKIX_LISTGETLENGTHFAILED);

                for (i = 0; i < numUserCheckers; i++) {

                    PKIX_CHECK(PKIX_List_GetItem
                        (userCheckers,
                        i,
                        (PKIX_PL_Object **) &userChecker,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK
                        (PKIX_CertChainChecker_IsForwardCheckingSupported
                        (userChecker, &supportForwardChecking, plContext),
                        PKIX_CERTCHAINCHECKERISFORWARDCHECKINGSUPPORTEDFAILED);

                    if (supportForwardChecking == PKIX_TRUE) {

                        PKIX_CHECK(PKIX_CertChainChecker_GetCheckCallback
                            (userChecker, &checkerCheck, plContext),
                            PKIX_CERTCHAINCHECKERGETCHECKCALLBACKFAILED);

                        pkixErrorResult =
                            checkerCheck(userChecker, candidateCert, NULL,
                                         &nbioContext, plContext);

                        ERROR_CHECK(PKIX_USERCHECKERCHECKFAILED);
                    }

                    PKIX_DECREF(userChecker);
                }
        }

        

        if (trusted) {
            PKIX_Boolean paramsNeeded = PKIX_FALSE;
            PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                       (candidateCert, &candidatePubKey, plContext),
                       PKIX_CERTGETSUBJECTPUBLICKEYFAILED);
            PKIX_CHECK(PKIX_PL_PublicKey_NeedsDSAParameters
                       (candidatePubKey, &paramsNeeded, plContext),
                       PKIX_PUBLICKEYNEEDSDSAPARAMETERSFAILED);
            if (paramsNeeded) {
                PKIX_ERROR(PKIX_MISSINGDSAPARAMETERS);
            }
        }
        
        
        if (revocationChecking) {
            if (!trusted) {
                if (state->revCheckDelayed) {
                    goto cleanup;
                } else {
                    PKIX_Boolean isSelfIssued = PKIX_FALSE;
                    PKIX_CHECK(
                        pkix_IsCertSelfIssued(candidateCert, &isSelfIssued,
                                              plContext),
                        PKIX_ISCERTSELFISSUEDFAILED);
                    if (isSelfIssued) {
                        state->revCheckDelayed = PKIX_TRUE;
                        goto cleanup;
                    }
                }
            }
            *pNeedsCRLChecking = PKIX_TRUE;
        }

cleanup:
        PKIX_DECREF(candidateCert);
        PKIX_DECREF(candidatePubKey);
        PKIX_DECREF(userChecker);

        PKIX_RETURN(BUILD);
}


































static PKIX_Error *
pkix_Build_ValidationCheckers(
        PKIX_ForwardBuilderState *state,
        PKIX_List *certChain,
        PKIX_TrustAnchor *anchor,
        PKIX_Boolean chainRevalidationStage,
        void *plContext)
{
        PKIX_List *checkers = NULL;
        PKIX_List *initialPolicies = NULL;
        PKIX_List *reversedCertChain = NULL;
        PKIX_List *buildCheckedCritExtOIDsList = NULL;
        PKIX_ProcessingParams *procParams = NULL;
        PKIX_PL_Cert *trustedCert = NULL;
        PKIX_PL_PublicKey *trustedPubKey = NULL;
        PKIX_CertChainChecker *sigChecker = NULL;
        PKIX_CertChainChecker *policyChecker = NULL;
        PKIX_CertChainChecker *userChecker = NULL;
        PKIX_CertChainChecker *checker = NULL;
        PKIX_CertSelector *certSelector = NULL;
        PKIX_List *userCheckerExtOIDs = NULL;
        PKIX_PL_OID *oid = NULL;
        PKIX_Boolean supportForwardChecking = PKIX_FALSE;
        PKIX_Boolean policyQualifiersRejected = PKIX_FALSE;
        PKIX_Boolean initialPolicyMappingInhibit = PKIX_FALSE;
        PKIX_Boolean initialAnyPolicyInhibit = PKIX_FALSE;
        PKIX_Boolean initialExplicitPolicy = PKIX_FALSE;
        PKIX_UInt32 numChainCerts;
        PKIX_UInt32 numCertCheckers;
        PKIX_UInt32 i;

        PKIX_ENTER(BUILD, "pkix_Build_ValidationCheckers");
        PKIX_NULLCHECK_THREE(state, certChain, anchor);

        PKIX_CHECK(PKIX_List_Create(&checkers, plContext),
                PKIX_LISTCREATEFAILED);

        PKIX_CHECK(PKIX_List_ReverseList
                (certChain, &reversedCertChain, plContext),
                PKIX_LISTREVERSELISTFAILED);

        PKIX_CHECK(PKIX_List_GetLength
                (reversedCertChain, &numChainCerts, plContext),
                PKIX_LISTGETLENGTHFAILED);

        procParams = state->buildConstants.procParams;

        




        if (chainRevalidationStage) {
            PKIX_CHECK(pkix_ExpirationChecker_Initialize
                       (state->buildConstants.testDate, &checker, plContext),
                       PKIX_EXPIRATIONCHECKERINITIALIZEFAILED);
            PKIX_CHECK(PKIX_List_AppendItem
                       (checkers, (PKIX_PL_Object *)checker, plContext),
                       PKIX_LISTAPPENDITEMFAILED);
            PKIX_DECREF(checker);
            
            PKIX_CHECK(PKIX_ProcessingParams_GetTargetCertConstraints
                       (procParams, &certSelector, plContext),
                    PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);

            PKIX_CHECK(pkix_TargetCertChecker_Initialize
                       (certSelector, numChainCerts, &checker, plContext),
                       PKIX_EXPIRATIONCHECKERINITIALIZEFAILED);
            PKIX_CHECK(PKIX_List_AppendItem
                       (checkers, (PKIX_PL_Object *)checker, plContext),
                       PKIX_LISTAPPENDITEMFAILED);
            PKIX_DECREF(checker);
        }

        PKIX_CHECK(PKIX_ProcessingParams_GetInitialPolicies
                (procParams, &initialPolicies, plContext),
                PKIX_PROCESSINGPARAMSGETINITIALPOLICIESFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetPolicyQualifiersRejected
                (procParams, &policyQualifiersRejected, plContext),
                PKIX_PROCESSINGPARAMSGETPOLICYQUALIFIERSREJECTEDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_IsPolicyMappingInhibited
                (procParams, &initialPolicyMappingInhibit, plContext),
                PKIX_PROCESSINGPARAMSISPOLICYMAPPINGINHIBITEDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_IsAnyPolicyInhibited
                (procParams, &initialAnyPolicyInhibit, plContext),
                PKIX_PROCESSINGPARAMSISANYPOLICYINHIBITEDFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_IsExplicitPolicyRequired
                (procParams, &initialExplicitPolicy, plContext),
                PKIX_PROCESSINGPARAMSISEXPLICITPOLICYREQUIREDFAILED);

        PKIX_CHECK(pkix_PolicyChecker_Initialize
                (initialPolicies,
                policyQualifiersRejected,
                initialPolicyMappingInhibit,
                initialExplicitPolicy,
                initialAnyPolicyInhibit,
                numChainCerts,
                &policyChecker,
                plContext),
                PKIX_POLICYCHECKERINITIALIZEFAILED);

        PKIX_CHECK(PKIX_List_AppendItem
                (checkers, (PKIX_PL_Object *)policyChecker, plContext),
                PKIX_LISTAPPENDITEMFAILED);

        



        PKIX_CHECK(PKIX_List_Create(&buildCheckedCritExtOIDsList, plContext),
                PKIX_LISTCREATEFAILED);

        for (i = 0; buildCheckedCritExtOIDs[i] != PKIX_UNKNOWN_OID; i++) {
                PKIX_CHECK(PKIX_PL_OID_Create
                        (buildCheckedCritExtOIDs[i], &oid, plContext),
                        PKIX_OIDCREATEFAILED);

                PKIX_CHECK(PKIX_List_AppendItem
                        (buildCheckedCritExtOIDsList,
                        (PKIX_PL_Object *) oid,
                        plContext),
                        PKIX_LISTAPPENDITEMFAILED);

                PKIX_DECREF(oid);
        }

        if (state->buildConstants.userCheckers != NULL) {

                PKIX_CHECK(PKIX_List_GetLength
                        (state->buildConstants.userCheckers,
                        &numCertCheckers,
                        plContext),
                        PKIX_LISTGETLENGTHFAILED);

                for (i = 0; i < numCertCheckers; i++) {

                        PKIX_CHECK(PKIX_List_GetItem
                            (state->buildConstants.userCheckers,
                            i,
                            (PKIX_PL_Object **) &userChecker,
                            plContext),
                            PKIX_LISTGETITEMFAILED);

                        PKIX_CHECK
                            (PKIX_CertChainChecker_IsForwardCheckingSupported
                            (userChecker, &supportForwardChecking, plContext),
                            PKIX_CERTCHAINCHECKERGETSUPPORTEDEXTENSIONSFAILED);

                        





                        if (supportForwardChecking == PKIX_TRUE) {

                          PKIX_CHECK
                            (PKIX_CertChainChecker_GetSupportedExtensions
                            (userChecker, &userCheckerExtOIDs, plContext),
                            PKIX_CERTCHAINCHECKERGETSUPPORTEDEXTENSIONSFAILED);

                          if (userCheckerExtOIDs != NULL) {
                            PKIX_CHECK(pkix_List_AppendList
                                (buildCheckedCritExtOIDsList,
                                userCheckerExtOIDs,
                                plContext),
                                PKIX_LISTAPPENDLISTFAILED);
                          }

                        } else {
                            PKIX_CHECK(PKIX_List_AppendItem
                                (checkers,
                                (PKIX_PL_Object *)userChecker,
                                plContext),
                                PKIX_LISTAPPENDITEMFAILED);
                        }

                        PKIX_DECREF(userCheckerExtOIDs);
                        PKIX_DECREF(userChecker);
                }
        }

        
        PKIX_CHECK(PKIX_TrustAnchor_GetTrustedCert
                   (anchor, &trustedCert, plContext),
                   PKIX_TRUSTANCHORGETTRUSTEDCERTFAILED);
        
        PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                   (trustedCert, &trustedPubKey, plContext),
                   PKIX_CERTGETSUBJECTPUBLICKEYFAILED);
        
        PKIX_CHECK(pkix_SignatureChecker_Initialize
                   (trustedPubKey,
                    numChainCerts,
                    &sigChecker,
                    plContext),
                   PKIX_SIGNATURECHECKERINITIALIZEFAILED);
        
        PKIX_CHECK(PKIX_List_AppendItem
                   (checkers,
                    (PKIX_PL_Object *)sigChecker,
                    plContext),
                   PKIX_LISTAPPENDITEMFAILED);

        PKIX_DECREF(state->reversedCertChain);
        PKIX_INCREF(reversedCertChain);
        state->reversedCertChain = reversedCertChain;
        PKIX_DECREF(state->checkedCritExtOIDs);
        PKIX_INCREF(buildCheckedCritExtOIDsList);
        state->checkedCritExtOIDs = buildCheckedCritExtOIDsList;
        PKIX_DECREF(state->checkerChain);
        state->checkerChain = checkers;
        checkers = NULL;
        state->certCheckedIndex = 0;
        state->checkerIndex = 0;
        state->revChecking = PKIX_FALSE;


cleanup:

        PKIX_DECREF(oid);
        PKIX_DECREF(reversedCertChain);
        PKIX_DECREF(buildCheckedCritExtOIDsList);
        PKIX_DECREF(checker);
        PKIX_DECREF(checkers);
        PKIX_DECREF(initialPolicies);
        PKIX_DECREF(trustedCert);
        PKIX_DECREF(trustedPubKey);
        PKIX_DECREF(certSelector);
        PKIX_DECREF(sigChecker);
        PKIX_DECREF(policyChecker);
        PKIX_DECREF(userChecker);
        PKIX_DECREF(userCheckerExtOIDs);

        PKIX_RETURN(BUILD);
}







































static PKIX_Error *
pkix_Build_ValidateEntireChain(
        PKIX_ForwardBuilderState *state,
        PKIX_TrustAnchor *anchor,
        void **pNBIOContext,
        PKIX_ValidateResult **pValResult,
        PKIX_VerifyNode *verifyNode,
        void *plContext)
{
        PKIX_UInt32 numChainCerts = 0;
        PKIX_PL_PublicKey *subjPubKey = NULL;
        PKIX_PolicyNode *policyTree = NULL;
        PKIX_ValidateResult *valResult = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_ValidateEntireChain");
        PKIX_NULLCHECK_FOUR(state, anchor, pNBIOContext, pValResult);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_List_GetLength
                (state->reversedCertChain, &numChainCerts, plContext),
                PKIX_LISTGETLENGTHFAILED);

        pkixErrorResult =
            pkix_CheckChain(state->reversedCertChain, numChainCerts, anchor,
                            state->checkerChain,
                            state->buildConstants.revChecker,
                            state->checkedCritExtOIDs,
                            state->buildConstants.procParams,
                            &state->certCheckedIndex, &state->checkerIndex,
                            &state->revChecking, &state->reasonCode,
                            &nbioContext, &subjPubKey, &policyTree, NULL,
                            plContext);

        if (nbioContext != NULL) {
                *pNBIOContext = nbioContext;
                goto cleanup;
        }

        ERROR_CHECK(PKIX_CHECKCHAINFAILED);

        if (state->reasonCode != 0) {
                PKIX_ERROR(PKIX_CHAINREJECTEDBYREVOCATIONCHECKER);
        }

        PKIX_CHECK(pkix_ValidateResult_Create
                (subjPubKey, anchor, policyTree, &valResult, plContext),
                PKIX_VALIDATERESULTCREATEFAILED);

        *pValResult = valResult;
        valResult = NULL;

cleanup:
        PKIX_DECREF(subjPubKey);
        PKIX_DECREF(policyTree);
        PKIX_DECREF(valResult);

        PKIX_RETURN(BUILD);
}



























static PKIX_Error *
pkix_Build_SortCandidateCerts(
        PKIX_List *candidates,
        PKIX_List **pSortedCandidates,
        void *plContext)
{
        PKIX_List *sortedList = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_SortCandidateCerts");
        PKIX_NULLCHECK_TWO(candidates, pSortedCandidates);

        








        PKIX_CHECK(pkix_List_BubbleSort
                (candidates,
                pkix_Build_SortCertComparator,
                &sortedList,
                plContext),
                PKIX_LISTBUBBLESORTFAILED);

        *pSortedCandidates = sortedList;

cleanup:

        PKIX_RETURN(BUILD);
}






















static PKIX_Error *
pkix_Build_BuildSelectorAndParams(
        PKIX_ForwardBuilderState *state,
        void *plContext)
{
        PKIX_ComCertSelParams *certSelParams = NULL;
        PKIX_CertSelector *certSel = NULL;
        PKIX_PL_X500Name *currentIssuer = NULL;
        PKIX_PL_ByteArray *authKeyId = NULL;
        PKIX_PL_Date *testDate = NULL;
        PKIX_CertSelector *callerCertSelector = NULL;
        PKIX_ComCertSelParams *callerComCertSelParams = NULL;
        PKIX_UInt32 reqKu = 0;
        PKIX_List   *reqEkuOids = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_BuildSelectorAndParams");
        PKIX_NULLCHECK_THREE(state, state->prevCert, state->traversedSubjNames);

        PKIX_CHECK(PKIX_PL_Cert_GetIssuer
                (state->prevCert, &currentIssuer, plContext),
                PKIX_CERTGETISSUERFAILED);

        PKIX_CHECK(PKIX_PL_Cert_GetAuthorityKeyIdentifier
                (state->prevCert, &authKeyId, plContext),
                PKIX_CERTGETAUTHORITYKEYIDENTIFIERFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_Create(&certSelParams, plContext),
                PKIX_COMCERTSELPARAMSCREATEFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_SetSubject
                (certSelParams, currentIssuer, plContext),
                PKIX_COMCERTSELPARAMSSETSUBJECTFAILED);

        if (authKeyId != NULL) {
            PKIX_CHECK(PKIX_ComCertSelParams_SetSubjKeyIdentifier
                    (certSelParams, authKeyId, plContext),
                    PKIX_COMCERTSELPARAMSSETSUBJKEYIDENTIFIERFAILED);
        }

        PKIX_INCREF(state->buildConstants.testDate);
        testDate = state->buildConstants.testDate;

        PKIX_CHECK(PKIX_ComCertSelParams_SetCertificateValid
                (certSelParams, testDate, plContext),
                PKIX_COMCERTSELPARAMSSETCERTIFICATEVALIDFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_SetBasicConstraints
                (certSelParams, state->traversedCACerts, plContext),
                PKIX_COMCERTSELPARAMSSETBASICCONSTRAINTSFAILED);

        PKIX_CHECK(PKIX_ComCertSelParams_SetPathToNames
                (certSelParams, state->traversedSubjNames, plContext),
                PKIX_COMCERTSELPARAMSSETPATHTONAMESFAILED);

        PKIX_CHECK(PKIX_ProcessingParams_GetTargetCertConstraints
                    (state->buildConstants.procParams,
                     &callerCertSelector, plContext),
                    PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);

        if (callerCertSelector != NULL) {

            
            PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                       (callerCertSelector, &callerComCertSelParams, plContext),
                       PKIX_CERTSELECTORGETCOMMONCERTSELECTORPARAMSFAILED);

            if (callerComCertSelParams != NULL) {
                PKIX_CHECK(PKIX_ComCertSelParams_GetExtendedKeyUsage
                           (callerComCertSelParams, &reqEkuOids, plContext),
                           PKIX_COMCERTSELPARAMSGETEXTENDEDKEYUSAGEFAILED);

                PKIX_CHECK(PKIX_ComCertSelParams_GetKeyUsage
                           (callerComCertSelParams, &reqKu, plContext),
                           PKIX_COMCERTSELPARAMSGETEXTENDEDKEYUSAGEFAILED);
            }
        }

        PKIX_CHECK(
            PKIX_ComCertSelParams_SetKeyUsage(certSelParams, reqKu,
                                              plContext),
            PKIX_COMCERTSELPARAMSSETKEYUSAGEFAILED);
        
        PKIX_CHECK(
            PKIX_ComCertSelParams_SetExtendedKeyUsage(certSelParams,
                                                      reqEkuOids,
                                                      plContext),
            PKIX_COMCERTSELPARAMSSETEXTKEYUSAGEFAILED);

        PKIX_CHECK(PKIX_CertSelector_Create
                (NULL, NULL, &state->certSel, plContext),
                PKIX_CERTSELECTORCREATEFAILED);

        PKIX_CHECK(PKIX_CertSelector_SetCommonCertSelectorParams
                (state->certSel, certSelParams, plContext),
                PKIX_CERTSELECTORSETCOMMONCERTSELECTORPARAMSFAILED);

        PKIX_CHECK(PKIX_List_Create(&state->candidateCerts, plContext),
                PKIX_LISTCREATEFAILED);

        state->certStoreIndex = 0;

cleanup:
        PKIX_DECREF(certSelParams);
        PKIX_DECREF(certSel);
        PKIX_DECREF(currentIssuer);
        PKIX_DECREF(authKeyId);
        PKIX_DECREF(testDate);
        PKIX_DECREF(reqEkuOids);
        PKIX_DECREF(callerComCertSelParams);
        PKIX_DECREF(callerCertSelector);

        PKIX_RETURN(BUILD);
}


static PKIX_Error*
pkix_Build_SelectCertsFromTrustAnchors(
    PKIX_List *trustAnchorsList,
    PKIX_ComCertSelParams *certSelParams,
    PKIX_List **pMatchList,
    void *plContext) 
{
    int anchorIndex = 0;
    PKIX_TrustAnchor *anchor = NULL;
    PKIX_PL_Cert *trustedCert = NULL;
    PKIX_List *matchList = NULL;
    PKIX_CertSelector *certSel = NULL;
    PKIX_CertSelector_MatchCallback selectorMatchCB = NULL;

    PKIX_ENTER(BUILD, "pkix_Build_SelectCertsFromTrustAnchors");
    
    PKIX_CHECK(PKIX_CertSelector_Create
               (NULL, NULL, &certSel, plContext),
               PKIX_CERTSELECTORCREATEFAILED);
    PKIX_CHECK(PKIX_CertSelector_SetCommonCertSelectorParams
               (certSel, certSelParams, plContext),
               PKIX_CERTSELECTORSETCOMMONCERTSELECTORPARAMSFAILED);
    PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
               (certSel, &selectorMatchCB, plContext),
               PKIX_CERTSELECTORGETMATCHCALLBACKFAILED);

    for (anchorIndex = 0;anchorIndex < trustAnchorsList->length; anchorIndex++) {
        PKIX_CHECK(
            PKIX_List_GetItem(trustAnchorsList,
                              anchorIndex,
                              (PKIX_PL_Object **)&anchor,
                              plContext),
            PKIX_LISTGETITEMFAILED);
        PKIX_CHECK(PKIX_TrustAnchor_GetTrustedCert
                   (anchor, &trustedCert, plContext),
                   PKIX_TRUSTANCHORGETTRUSTEDCERTFAILED);
        pkixErrorResult =
            (*selectorMatchCB)(certSel, trustedCert, plContext);
        if (!pkixErrorResult) {
            if (!matchList) {
                PKIX_CHECK(PKIX_List_Create(&matchList,
                                            plContext),
                           PKIX_LISTCREATEFAILED);
            }
            PKIX_CHECK(
                PKIX_List_AppendItem(matchList,
                    (PKIX_PL_Object*)trustedCert,
                                     plContext),
                PKIX_LISTAPPENDITEMFAILED);
        } else {
            PKIX_DECREF(pkixErrorResult);
        }
        PKIX_DECREF(trustedCert);
        PKIX_DECREF(anchor);
     }
    
    *pMatchList = matchList;
    matchList = NULL;

cleanup:
    PKIX_DECREF(matchList);
    PKIX_DECREF(trustedCert);
    PKIX_DECREF(anchor);
    PKIX_DECREF(certSel);
    
    PKIX_RETURN(BUILD);
}


static PKIX_Error*
pkix_Build_RemoveDupUntrustedCerts(
    PKIX_List *trustedCertList,
    PKIX_List *certsFound,
    void *plContext)
{
    PKIX_UInt32 trustIndex;
    PKIX_PL_Cert *trustCert = NULL, *cert = NULL;

    PKIX_ENTER(BUILD, "pkix_Build_RemoveDupUntrustedCerts");
    if (trustedCertList == NULL || certsFound == NULL) {
        goto cleanup;
    }
    for (trustIndex = 0;trustIndex < trustedCertList->length;
        trustIndex++) {
        PKIX_UInt32 certIndex = 0;
        PKIX_CHECK(
            PKIX_List_GetItem(trustedCertList,
                              trustIndex,
                              (PKIX_PL_Object **)&trustCert,
                              plContext),
            PKIX_LISTGETITEMFAILED);
        
        while (certIndex < certsFound->length) {
            PKIX_Boolean result = PKIX_FALSE;
            PKIX_DECREF(cert);
            PKIX_CHECK(
                PKIX_List_GetItem(certsFound, certIndex,
                                  (PKIX_PL_Object **)&cert,
                                  plContext),
                PKIX_LISTGETITEMFAILED);
            PKIX_CHECK(
                PKIX_PL_Object_Equals((PKIX_PL_Object *)trustCert,
                                      (PKIX_PL_Object *)cert,
                                      &result,
                                      plContext),
                PKIX_OBJECTEQUALSFAILED);
            if (!result) {
                certIndex += 1;
                continue;
            }
            PKIX_CHECK(
                PKIX_List_DeleteItem(certsFound, certIndex,
                                     plContext),
                PKIX_LISTDELETEITEMFAILED);
        }
        PKIX_DECREF(trustCert);
    }
cleanup:
    PKIX_DECREF(cert);
    PKIX_DECREF(trustCert);

    PKIX_RETURN(BUILD);
}







































static PKIX_Error *
pkix_Build_GatherCerts(
        PKIX_ForwardBuilderState *state,
        PKIX_ComCertSelParams *certSelParams,
        void **pNBIOContext,
        void *plContext)
{
        PKIX_Boolean certStoreIsCached = PKIX_FALSE;
        PKIX_Boolean certStoreIsLocal = PKIX_FALSE;
        PKIX_Boolean foundInCache = PKIX_FALSE;
        PKIX_CertStore *certStore = NULL;
        PKIX_CertStore_CertCallback getCerts = NULL;
        PKIX_List *certsFound = NULL;
        PKIX_List *trustedCertList = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_GatherCerts");
        PKIX_NULLCHECK_THREE(state, certSelParams, pNBIOContext);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        PKIX_DECREF(state->candidateCerts);

        while (state->certStoreIndex < state->buildConstants.numCertStores) {

                
                PKIX_CHECK(PKIX_List_GetItem
                        (state->buildConstants.certStores,
                        state->certStoreIndex,
                        (PKIX_PL_Object **)&certStore,
                        plContext),
                        PKIX_LISTGETITEMFAILED);

                PKIX_CHECK(PKIX_CertStore_GetLocalFlag
                           (certStore, &certStoreIsLocal, plContext),
                           PKIX_CERTSTOREGETLOCALFLAGFAILED);

                if (state->useOnlyLocal == certStoreIsLocal) {
                    
                    if (state->status == BUILD_GATHERPENDING) {
                        certStoreIsCached = PKIX_FALSE;
                        foundInCache = PKIX_FALSE;
                    } else {
                        PKIX_CHECK(PKIX_CertStore_GetCertStoreCacheFlag
                                (certStore, &certStoreIsCached, plContext),
                                PKIX_CERTSTOREGETCERTSTORECACHEFLAGFAILED);

                        if (certStoreIsCached) {
                        









                                PKIX_CHECK(pkix_CacheCert_Lookup
                                        (certStore,
                                        certSelParams,
                                        state->buildConstants.testDate,
                                        &foundInCache,
                                        &certsFound,
                                        plContext),
                                        PKIX_CACHECERTCHAINLOOKUPFAILED);

                        }
                    }

                    






                    if (!foundInCache) {

                        if (nbioContext == NULL) {
                                PKIX_CHECK(PKIX_CertStore_GetCertCallback
                                        (certStore, &getCerts, plContext),
                                        PKIX_CERTSTOREGETCERTCALLBACKFAILED);

                                PKIX_CHECK(getCerts
                                        (certStore,
                                        state->certSel,
                                        state->verifyNode,
                                        &nbioContext,
                                        &certsFound,
                                        plContext),
                                        PKIX_GETCERTSFAILED);
                        } else {
                                PKIX_CHECK(PKIX_CertStore_CertContinue
                                        (certStore,
                                        state->certSel,
                                        state->verifyNode,
                                        &nbioContext,
                                        &certsFound,
                                        plContext),
                                        PKIX_CERTSTORECERTCONTINUEFAILED);
                        }

                        if (certStoreIsCached && certsFound) {

                                PKIX_CHECK(pkix_CacheCert_Add
                                        (certStore,
                                        certSelParams,
                                        certsFound,
                                        plContext),
                                        PKIX_CACHECERTADDFAILED);
                        }
                    }

                    



                    if (certsFound == NULL) {
                        state->status = BUILD_GATHERPENDING;
                        *pNBIOContext = nbioContext;
                        goto cleanup;
                    }
                }

                
                PKIX_DECREF(certStore);
                ++(state->certStoreIndex);
        }

        if (certsFound && certsFound->length > 1) {
            PKIX_List *sorted = NULL;
            
            
            PKIX_CHECK(pkix_Build_SortCandidateCerts
                       (certsFound, &sorted, plContext),
                       PKIX_BUILDSORTCANDIDATECERTSFAILED);
            PKIX_DECREF(certsFound);
            certsFound = sorted;
        }

        PKIX_CHECK(
            pkix_Build_SelectCertsFromTrustAnchors(
                state->buildConstants.anchors,
                certSelParams, &trustedCertList,
                plContext),
            PKIX_FAILTOSELECTCERTSFROMANCHORS);
        PKIX_CHECK(
            pkix_Build_RemoveDupUntrustedCerts(trustedCertList,
                                               certsFound,
                                               plContext),
            PKIX_REMOVEDUPUNTRUSTEDCERTSFAILED);

        PKIX_CHECK(
            pkix_List_MergeLists(trustedCertList,
                                 certsFound,
                                 &state->candidateCerts,
                                 plContext),
            PKIX_LISTMERGEFAILED);

        
        PKIX_CHECK(PKIX_List_GetLength
                (state->candidateCerts, &state->numCerts, plContext),
                PKIX_LISTGETLENGTHFAILED);

        state->certIndex = 0;

cleanup:
        PKIX_DECREF(trustedCertList);
        PKIX_DECREF(certStore);
        PKIX_DECREF(certsFound);

        PKIX_RETURN(BUILD);
}






















static PKIX_Error *
pkix_Build_UpdateDate(
        PKIX_ForwardBuilderState *state,
        void *plContext)
{
        PKIX_Boolean canBeCached = PKIX_FALSE;
        PKIX_Int32 comparison = 0;
        PKIX_PL_Date *notAfter = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_UpdateDate");
        PKIX_NULLCHECK_ONE(state);

        PKIX_CHECK(PKIX_PL_Cert_GetCacheFlag
                (state->candidateCert, &canBeCached, plContext),
                PKIX_CERTGETCACHEFLAGFAILED);

        state->canBeCached = state->canBeCached && canBeCached;
        if (state->canBeCached == PKIX_TRUE) {

                




                PKIX_CHECK(PKIX_PL_Cert_GetValidityNotAfter
                        (state->candidateCert, &notAfter, plContext),
                        PKIX_CERTGETVALIDITYNOTAFTERFAILED);

                if (state->validityDate == NULL) {
                        state->validityDate = notAfter;
                        notAfter = NULL;
                } else {
                        PKIX_CHECK(PKIX_PL_Object_Compare
                                ((PKIX_PL_Object *)state->validityDate,
                                (PKIX_PL_Object *)notAfter,
                                &comparison,
                                plContext),
                                PKIX_OBJECTCOMPARATORFAILED);
                        if (comparison > 0) {
                                PKIX_DECREF(state->validityDate);
                                state->validityDate = notAfter;
                                notAfter = NULL;
                        }
                }
        }

cleanup:

        PKIX_DECREF(notAfter);

        PKIX_RETURN(BUILD);
}


static void
pkix_PrepareForwardBuilderStateForAIA(
        PKIX_ForwardBuilderState *state)
{
        PORT_Assert(state->useOnlyLocal == PKIX_TRUE);
        state->useOnlyLocal = PKIX_FALSE;
        state->certStoreIndex = 0;
        state->numFanout = state->buildConstants.maxFanout;
        state->status = BUILD_TRYAIA;
}



















































































static PKIX_Error *
pkix_BuildForwardDepthFirstSearch(
        void **pNBIOContext,
        PKIX_ForwardBuilderState *state,
        PKIX_ValidateResult **pValResult,
        void *plContext)
{
        PKIX_Boolean outOfOptions = PKIX_FALSE;
        PKIX_Boolean trusted = PKIX_FALSE;
        PKIX_Boolean isSelfIssued = PKIX_FALSE;
        PKIX_Boolean canBeCached = PKIX_FALSE;
        PKIX_Boolean revocationCheckingExists = PKIX_FALSE;
        PKIX_Boolean needsCRLChecking = PKIX_FALSE;
        PKIX_Boolean ioPending = PKIX_FALSE;
        PKIX_PL_Date *validityDate = NULL;
        PKIX_PL_Date *currTime  = NULL;
        PKIX_Int32 childTraversedCACerts = 0;
        PKIX_UInt32 numSubjectNames = 0;
        PKIX_UInt32 numChained = 0;
        PKIX_Int32 cmpTimeResult = 0;
        PKIX_UInt32 i = 0;
        PKIX_UInt32 certsSoFar = 0;
        PKIX_List *childTraversedSubjNames = NULL;
        PKIX_List *subjectNames = NULL;
        PKIX_List *unfilteredCerts = NULL;
        PKIX_List *filteredCerts = NULL;
        PKIX_PL_Object *subjectName = NULL;
        PKIX_ValidateResult *valResult = NULL;
        PKIX_ForwardBuilderState *childState = NULL;
        PKIX_ForwardBuilderState *parentState = NULL;
        PKIX_PL_Object *revCheckerState = NULL;
        PKIX_ComCertSelParams *certSelParams = NULL;
        PKIX_TrustAnchor *trustAnchor = NULL;
        PKIX_PL_Cert *trustedCert = NULL;
        PKIX_VerifyNode *verifyNode = NULL;
        PKIX_Error *verifyError = NULL;
        PKIX_Error *finalError = NULL;
        void *nbio = NULL;
        PKIX_UInt32 numIterations = 0;

        PKIX_ENTER(BUILD, "pkix_BuildForwardDepthFirstSearch");
        PKIX_NULLCHECK_THREE(pNBIOContext, state, pValResult);

        nbio = *pNBIOContext;
        *pNBIOContext = NULL;
        PKIX_INCREF(state->validityDate);
        validityDate = state->validityDate;
        canBeCached = state->canBeCached;
        PKIX_DECREF(*pValResult);

        



        while (outOfOptions == PKIX_FALSE) {
            




            if (numIterations++ > 250)
                    PKIX_ERROR(PKIX_TIMECONSUMEDEXCEEDSRESOURCELIMITS);

            if (state->buildConstants.maxTime != 0) {
                    PKIX_DECREF(currTime);
                    PKIX_CHECK(PKIX_PL_Date_Create_UTCTime
                            (NULL, &currTime, plContext),
                            PKIX_DATECREATEUTCTIMEFAILED);

                    PKIX_CHECK(PKIX_PL_Object_Compare
                             ((PKIX_PL_Object *)state->buildConstants.timeLimit,
                             (PKIX_PL_Object *)currTime,
                             &cmpTimeResult,
                             plContext),
                             PKIX_OBJECTCOMPARATORFAILED);

                    if (cmpTimeResult < 0) {
                        if (state->verifyNode != NULL) {
                                PKIX_ERROR_CREATE
                                    (BUILD,
                                    PKIX_TIMECONSUMEDEXCEEDSRESOURCELIMITS,
                                    verifyError);
                                PKIX_CHECK_FATAL(pkix_VerifyNode_SetError
                                    (state->verifyNode,
                                    verifyError,
                                    plContext),
                                    PKIX_VERIFYNODESETERRORFAILED);
                                PKIX_DECREF(finalError);
                                finalError = verifyError;
                                verifyError = NULL;
                        }
                        
                        PKIX_ERROR(PKIX_TIMECONSUMEDEXCEEDSRESOURCELIMITS);
                    }
            }

            if (state->status == BUILD_INITIAL) {

                PKIX_CHECK(pkix_Build_BuildSelectorAndParams(state, plContext),
                        PKIX_BUILDBUILDSELECTORANDPARAMSFAILED);

                



                if (state->buildConstants.numHintCerts > 0) {
                        
                        PKIX_CHECK(PKIX_List_GetLength
                                (state->trustChain, &certsSoFar, plContext),
                                PKIX_LISTGETLENGTHFAILED);

                        
                        certsSoFar--;

                        
                        if (certsSoFar >= state->buildConstants.numHintCerts) {
                                state->status = BUILD_TRYAIA;
                        } else {
                                



                                PKIX_DECREF(state->candidateCert);
                                PKIX_CHECK(PKIX_List_GetItem
                                    (state->buildConstants.hintCerts,
                                    certsSoFar,
                                    (PKIX_PL_Object **)&state->candidateCert,
                                    plContext),
                                    PKIX_LISTGETITEMFAILED);

                                PKIX_CHECK(PKIX_List_AppendItem
                                    (state->candidateCerts,
                                    (PKIX_PL_Object *)state->candidateCert,
                                    plContext),
                                    PKIX_LISTAPPENDITEMFAILED);

                                state->numCerts = 1;
                                state->usingHintCerts = PKIX_TRUE;
                                state->status = BUILD_CERTVALIDATING;
                        }
                } else {
                        state->status = BUILD_TRYAIA;
                }

            }

            if (state->status == BUILD_TRYAIA) {
                if (state->useOnlyLocal == PKIX_TRUE) {
                        state->status = BUILD_COLLECTINGCERTS;
                } else {
                        state->status = BUILD_AIAPENDING;
                }
            }

            if (state->status == BUILD_AIAPENDING &&
                state->buildConstants.aiaMgr) {
                pkixErrorResult = PKIX_PL_AIAMgr_GetAIACerts
                        (state->buildConstants.aiaMgr,
                        state->prevCert,
                        &nbio,
                        &unfilteredCerts,
                         plContext);

                if (nbio != NULL) {
                        
                        *pNBIOContext = nbio;
                        goto cleanup;
                }
                state->numCerts = 0;
                if (pkixErrorResult) {
                    pkixErrorClass = pkixErrorResult->errClass;
                    if (pkixErrorClass == PKIX_FATAL_ERROR) {
                        goto fatal;
                    }
                    PKIX_DECREF(finalError);
                    finalError = pkixErrorResult;
                    pkixErrorResult = NULL;
                    if (state->verifyNode != NULL) {
                        




                        PKIX_CHECK_FATAL(pkix_VerifyNode_Create
                                         (state->prevCert,
                                          0, NULL,
                                          &verifyNode,
                                          plContext),
                                         PKIX_VERIFYNODECREATEFAILED);
                        PKIX_CHECK_FATAL(pkix_VerifyNode_SetError
                                         (verifyNode, finalError, plContext),
                                         PKIX_VERIFYNODESETERRORFAILED);
                        PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                         (state->verifyNode,
                                          verifyNode,
                                          plContext),
                                         PKIX_VERIFYNODEADDTOTREEFAILED);
                        PKIX_DECREF(verifyNode);
                    }
                }
#ifdef PKIX_BUILDDEBUG
                
                {
                                PKIX_PL_String *unString;
                                char *unAscii;
                                PKIX_UInt32 length;
                                PKIX_TOSTRING
                                        ((PKIX_PL_Object*)unfilteredCerts,
                                        &unString,
                                        plContext,
                                        PKIX_OBJECTTOSTRINGFAILED);

                                PKIX_CHECK(PKIX_PL_String_GetEncoded
                                        (unString,
                                        PKIX_ESCASCII,
                                        (void **)&unAscii,
                                        &length,
                                        plContext),
                                        PKIX_STRINGGETENCODEDFAILED);

                                PKIX_DEBUG_ARG
                                        ("unfilteredCerts = %s\n", unAscii);
                                PKIX_DECREF(unString);
                                PKIX_FREE(unAscii);
                }
#endif

                
                if (unfilteredCerts) {
                        PKIX_CHECK(pkix_CertSelector_Select
                                (state->certSel,
                                unfilteredCerts,
                                &filteredCerts,
                                plContext),
                                PKIX_CERTSELECTORSELECTFAILED);

                        PKIX_DECREF(unfilteredCerts);

                        PKIX_CHECK(PKIX_List_GetLength
                                (filteredCerts, &(state->numCerts), plContext),
                                PKIX_LISTGETLENGTHFAILED);

#ifdef PKIX_BUILDDEBUG
                
                {
                        PKIX_PL_String *unString;
                        char *unAscii;
                        PKIX_UInt32 length;
                        PKIX_TOSTRING
                                ((PKIX_PL_Object*)filteredCerts,
                                &unString,
                                plContext,
                                PKIX_OBJECTTOSTRINGFAILED);

                        PKIX_CHECK(PKIX_PL_String_GetEncoded
                                    (unString,
                                    PKIX_ESCASCII,
                                    (void **)&unAscii,
                                    &length,
                                    plContext),
                                    PKIX_STRINGGETENCODEDFAILED);

                        PKIX_DEBUG_ARG("filteredCerts = %s\n", unAscii);
                        PKIX_DECREF(unString);
                        PKIX_FREE(unAscii);
                }
#endif

                        PKIX_DECREF(state->candidateCerts);
                        state->candidateCerts = filteredCerts;
                        state->certIndex = 0;
                        filteredCerts = NULL;
                }

                
                if (state->numCerts > 0) {
                        state->status = BUILD_CERTVALIDATING;
                } else {
                        state->status = BUILD_COLLECTINGCERTS;
                }
            }

            PKIX_DECREF(certSelParams);
            PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                (state->certSel, &certSelParams, plContext),
                PKIX_CERTSELECTORGETCOMMONCERTSELECTORPARAMSFAILED);

            
            if ((state->status == BUILD_COLLECTINGCERTS) ||
                (state->status == BUILD_GATHERPENDING)) {

#if PKIX_FORWARDBUILDERSTATEDEBUG
                PKIX_CHECK(pkix_ForwardBuilderState_DumpState
                        (state, plContext),
                        PKIX_FORWARDBUILDERSTATEDUMPSTATEFAILED);
#endif

                PKIX_CHECK(pkix_Build_GatherCerts
                        (state, certSelParams, &nbio, plContext),
                        PKIX_BUILDGATHERCERTSFAILED);

                if (nbio != NULL) {
                        
                        *pNBIOContext = nbio;
                        goto cleanup;
                }

                
                if (state->numCerts > 0) {
                        state->status = BUILD_CERTVALIDATING;
                } else {
                        state->status = BUILD_ABANDONNODE;
                }
            }

            

#if PKIX_FORWARDBUILDERSTATEDEBUG
            PKIX_CHECK(pkix_ForwardBuilderState_DumpState(state, plContext),
                    PKIX_FORWARDBUILDERSTATEDUMPSTATEFAILED);
#endif

            if (state->status == BUILD_CERTVALIDATING) {
                    revocationCheckingExists =
                        (state->buildConstants.revChecker != NULL);

                    PKIX_DECREF(state->candidateCert);
                    PKIX_CHECK(PKIX_List_GetItem
                            (state->candidateCerts,
                            state->certIndex,
                            (PKIX_PL_Object **)&(state->candidateCert),
                            plContext),
                            PKIX_LISTGETITEMFAILED);

                    if ((state->verifyNode) != NULL) {
                            PKIX_CHECK_FATAL(pkix_VerifyNode_Create
                                    (state->candidateCert,
                                    0,
                                    NULL,
                                    &verifyNode,
                                    plContext),
                                    PKIX_VERIFYNODECREATEFAILED);
                    }

                    
                    verifyError = pkix_Build_VerifyCertificate
                            (state,
                            state->buildConstants.userCheckers,
                            revocationCheckingExists,
                            &trusted,
                            &needsCRLChecking,
                            verifyNode,
                            plContext);

                    if (verifyError) {
                            pkixTempErrorReceived = PKIX_TRUE;
                            pkixErrorClass = verifyError->errClass;
                            if (pkixErrorClass == PKIX_FATAL_ERROR) {
                                pkixErrorResult = verifyError;
                                verifyError = NULL;
                                goto fatal;
                            }
                    }

                    if (PKIX_ERROR_RECEIVED) {
                            if (state->verifyNode != NULL) {
                                PKIX_CHECK_FATAL(pkix_VerifyNode_SetError
                                    (verifyNode, verifyError, plContext),
                                    PKIX_VERIFYNODESETERRORFAILED);
                                PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                        (state->verifyNode,
                                        verifyNode,
                                        plContext),
                                        PKIX_VERIFYNODEADDTOTREEFAILED);
                                PKIX_DECREF(verifyNode);
                            }
                            pkixTempErrorReceived = PKIX_FALSE;
                            PKIX_DECREF(finalError);
                            finalError = verifyError;
                            verifyError = NULL;
                            if (state->certLoopingDetected) {
                                PKIX_ERROR
                                    (PKIX_LOOPDISCOVEREDDUPCERTSNOTALLOWED);
                            }
                            state->status = BUILD_GETNEXTCERT;
                    } else if (needsCRLChecking) {
                            state->status = BUILD_CRLPREP;
                    } else {
                            state->status = BUILD_DATEPREP;
                    }
            }

            if (state->status == BUILD_CRLPREP) {
                PKIX_RevocationStatus revStatus;
                PKIX_UInt32 reasonCode;

                verifyError =
                    PKIX_RevocationChecker_Check(
                             state->prevCert, state->candidateCert,
                             state->buildConstants.revChecker,
                             state->buildConstants.procParams,
                             PKIX_FALSE,
                             (state->parentState == NULL) ?
                                              PKIX_TRUE : PKIX_FALSE,
                             &revStatus, &reasonCode,
                             &nbio, plContext);
                if (nbio != NULL) {
                    *pNBIOContext = nbio;
                    goto cleanup;
                }
                if (revStatus == PKIX_RevStatus_Revoked || verifyError) {
                    if (!verifyError) {
                        


                        PKIX_ERROR_CREATE(VALIDATE, PKIX_CERTIFICATEREVOKED,
                                          verifyError);
                    }
                    if (state->verifyNode != NULL) {
                            PKIX_CHECK_FATAL(pkix_VerifyNode_SetError
                                    (verifyNode, verifyError, plContext),
                                    PKIX_VERIFYNODESETERRORFAILED);
                            PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                    (state->verifyNode,
                                    verifyNode,
                                    plContext),
                                    PKIX_VERIFYNODEADDTOTREEFAILED);
                            PKIX_DECREF(verifyNode);
                    }
                    PKIX_DECREF(finalError);
                    finalError = verifyError;
                    verifyError = NULL;
                    if (state->certLoopingDetected) {
                            PKIX_ERROR
                                (PKIX_LOOPDISCOVEREDDUPCERTSNOTALLOWED);
                    }
                    state->status = BUILD_GETNEXTCERT;
                } else {
                    state->status = BUILD_DATEPREP;
                }
            }

            if (state->status == BUILD_DATEPREP) {
                    
                    PKIX_CHECK(pkix_Build_UpdateDate(state, plContext),
                            PKIX_BUILDUPDATEDATEFAILED);
    
                    canBeCached = state->canBeCached;
                    PKIX_DECREF(validityDate);
                    PKIX_INCREF(state->validityDate);
                    validityDate = state->validityDate;
                    if (trusted == PKIX_TRUE) {
                            state->status = BUILD_CHECKTRUSTED;
                    } else {
                            state->status = BUILD_ADDTOCHAIN;
                    }
            }

            if (state->status == BUILD_CHECKTRUSTED) {

                    



                    PKIX_CHECK(PKIX_TrustAnchor_CreateWithCert
                      (state->candidateCert,
                      &trustAnchor,
                      plContext),
                      PKIX_TRUSTANCHORCREATEWITHCERTFAILED);

                    PKIX_CHECK(pkix_Build_ValidationCheckers
                      (state,
                      state->trustChain,
                      trustAnchor,
                      PKIX_FALSE, 


                      plContext),
                      PKIX_BUILDVALIDATIONCHECKERSFAILED);

                    state->status = BUILD_CHECKTRUSTED2;
            }

            if (state->status == BUILD_CHECKTRUSTED2) {
                    verifyError = 
                        pkix_Build_ValidateEntireChain(state,
                                                       trustAnchor,
                                                       &nbio, &valResult,
                                                       verifyNode,
                                                       plContext);
                    if (nbio != NULL) {
                            
                            goto cleanup;
                    } else {
                            
                            if (verifyError) {
                                pkixTempErrorReceived = PKIX_TRUE;
                                pkixErrorClass = verifyError->errClass;
                                if (pkixErrorClass == PKIX_FATAL_ERROR) {
                                    pkixErrorResult = verifyError;
                                    verifyError = NULL;
                                    goto fatal;
                                }
                            }
                            if (state->verifyNode != NULL) {
                                PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                        (state->verifyNode,
                                        verifyNode,
                                        plContext),
                                        PKIX_VERIFYNODEADDTOTREEFAILED);
                                PKIX_DECREF(verifyNode);
                            }
                            if (!PKIX_ERROR_RECEIVED) {
                                *pValResult = valResult;
                                valResult = NULL;
                                
                                state->status = BUILD_CHECKTRUSTED;
                                goto cleanup;
                            }
                            PKIX_DECREF(finalError);
                            finalError = verifyError;
                            verifyError = NULL;
                            

                            pkixTempErrorReceived = PKIX_FALSE;
                            PKIX_DECREF(trustAnchor);
                    }

                    



                    if (state->certLoopingDetected) {
                            PKIX_DECREF(verifyError);
                            PKIX_ERROR_CREATE(BUILD, 
                                         PKIX_LOOPDISCOVEREDDUPCERTSNOTALLOWED,
                                         verifyError);
                            PKIX_CHECK_FATAL(
                                pkix_VerifyNode_SetError(state->verifyNode,
                                                         verifyError,
                                                         plContext),
                                PKIX_VERIFYNODESETERRORFAILED);
                            PKIX_DECREF(verifyError);
                    }
                    state->status = BUILD_GETNEXTCERT;
            }

            




            if (state->status == BUILD_ADDTOCHAIN) {
                    PKIX_CHECK(PKIX_List_AppendItem
                            (state->trustChain,
                            (PKIX_PL_Object *)state->candidateCert,
                            plContext),
                            PKIX_LISTAPPENDITEMFAILED);

                    state->status = BUILD_EXTENDCHAIN;
            }

            if (state->status == BUILD_EXTENDCHAIN) {

                    
                    if ((state->buildConstants.maxDepth != 0) &&
                        (state->numDepth <= 1)) {

                        if (state->verifyNode != NULL) {
                                PKIX_ERROR_CREATE
                                    (BUILD,
                                    PKIX_DEPTHWOULDEXCEEDRESOURCELIMITS,
                                    verifyError);
                                PKIX_CHECK_FATAL(pkix_VerifyNode_SetError
                                    (verifyNode, verifyError, plContext),
                                    PKIX_VERIFYNODESETERRORFAILED);
                                PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                    (state->verifyNode, verifyNode, plContext),
                                    PKIX_VERIFYNODEADDTOTREEFAILED);
                                PKIX_DECREF(verifyNode);
                                PKIX_DECREF(finalError);
                                finalError = verifyError;
                                verifyError = NULL;
                        }
                        
                        PKIX_ERROR(PKIX_DEPTHWOULDEXCEEDRESOURCELIMITS);
                    }

                    PKIX_CHECK(pkix_IsCertSelfIssued
                            (state->candidateCert, &isSelfIssued, plContext),
                            PKIX_ISCERTSELFISSUEDFAILED);
                 
                    PKIX_CHECK(PKIX_PL_Object_Duplicate
                            ((PKIX_PL_Object *)state->traversedSubjNames,
                            (PKIX_PL_Object **)&childTraversedSubjNames,
                            plContext),
                            PKIX_OBJECTDUPLICATEFAILED);
         
                    if (isSelfIssued) {
                            childTraversedCACerts = state->traversedCACerts;
                    } else {
                            childTraversedCACerts = state->traversedCACerts + 1;
                 
                            PKIX_CHECK(PKIX_PL_Cert_GetAllSubjectNames
                                (state->candidateCert,
                                &subjectNames,
                                plContext),
                                PKIX_CERTGETALLSUBJECTNAMESFAILED);
                 
                            if (subjectNames) {
                                PKIX_CHECK(PKIX_List_GetLength
                                    (subjectNames,
                                    &numSubjectNames,
                                    plContext),
                                    PKIX_LISTGETLENGTHFAILED);
         
                            } else {
                                numSubjectNames = 0;
                            }

                            for (i = 0; i < numSubjectNames; i++) {
                                PKIX_CHECK(PKIX_List_GetItem
                                    (subjectNames,
                                    i,
                                    &subjectName,
                                    plContext),
                                    PKIX_LISTGETITEMFAILED);
                                PKIX_NULLCHECK_ONE
                                    (state->traversedSubjNames);
                                PKIX_CHECK(PKIX_List_AppendItem
                                    (state->traversedSubjNames,
                                    subjectName,
                                    plContext),
                                    PKIX_LISTAPPENDITEMFAILED);
                                PKIX_DECREF(subjectName);
                            }
                            PKIX_DECREF(subjectNames);
                        }
            
                        PKIX_CHECK(pkix_ForwardBuilderState_Create
                            (childTraversedCACerts,
                            state->buildConstants.maxFanout,
                            state->numDepth - 1,
                            state->revCheckDelayed,
                            canBeCached,
                            validityDate,
                            state->candidateCert,
                            childTraversedSubjNames,
                            state->trustChain,
                            state,
                            &childState,
                            plContext),
                            PKIX_FORWARDBUILDSTATECREATEFAILED);

                        PKIX_DECREF(childTraversedSubjNames);
                        PKIX_DECREF(certSelParams);
                        childState->verifyNode = verifyNode;
                        verifyNode = NULL;
                        PKIX_DECREF(state);
                        state = childState; 
                        childState = NULL;
                        continue; 
            }

            if (state->status == BUILD_GETNEXTCERT) {
                    pkixTempErrorReceived = PKIX_FALSE;
                    PKIX_DECREF(state->candidateCert);

                    



                    if (state->usingHintCerts == PKIX_TRUE) {
                            PKIX_DECREF(state->candidateCerts);
                            PKIX_CHECK(PKIX_List_Create
                                (&state->candidateCerts, plContext),
                                PKIX_LISTCREATEFAILED);

                            state->numCerts = 0;
                            state->usingHintCerts = PKIX_FALSE;
                            state->status = BUILD_TRYAIA;
                            continue;
                    } else if (++(state->certIndex) < (state->numCerts)) {
                            if ((state->buildConstants.maxFanout != 0) &&
                                (--(state->numFanout) == 0)) {

                                if (state->verifyNode != NULL) {
                                        PKIX_ERROR_CREATE
                                            (BUILD,
                                            PKIX_FANOUTEXCEEDSRESOURCELIMITS,
                                            verifyError);
                                        PKIX_CHECK_FATAL
                                            (pkix_VerifyNode_SetError
                                            (state->verifyNode,
                                            verifyError,
                                            plContext),
                                            PKIX_VERIFYNODESETERRORFAILED);
                                        PKIX_DECREF(finalError);
                                        finalError = verifyError;
                                        verifyError = NULL;
                                }
                                
                                PKIX_ERROR
                                        (PKIX_FANOUTEXCEEDSRESOURCELIMITS);
                            }
                            state->status = BUILD_CERTVALIDATING;
                            continue;
                    }
            }

            





            if (state->useOnlyLocal == PKIX_TRUE) {
                pkix_PrepareForwardBuilderStateForAIA(state);
            } else do {
                if (state->parentState == NULL) {
                        
                        outOfOptions = PKIX_TRUE;
                } else {
                        




                        PKIX_CHECK(PKIX_List_GetLength
                                (state->trustChain, &numChained, plContext),
                                PKIX_LISTGETLENGTHFAILED);
                        PKIX_CHECK(PKIX_List_DeleteItem
                                (state->trustChain, numChained - 1, plContext),
                                PKIX_LISTDELETEITEMFAILED);
                        
                        


                        if (!state->verifyNode) {
                            PKIX_CHECK_FATAL(
                                pkix_VerifyNode_Create(state->prevCert,
                                                       0, NULL, 
                                                       &state->verifyNode,
                                                       plContext),
                                PKIX_VERIFYNODECREATEFAILED);
                        }
                        
                        PKIX_DECREF(verifyError);
                        PKIX_ERROR_CREATE(BUILD, PKIX_SECERRORUNKNOWNISSUER,
                                          verifyError);
                        PKIX_CHECK_FATAL(
                            pkix_VerifyNode_SetError(state->verifyNode,
                                                     verifyError,
                                                     plContext),
                            PKIX_VERIFYNODESETERRORFAILED);
                        PKIX_DECREF(verifyError);

                        PKIX_INCREF(state->parentState);
                        parentState = state->parentState;
                        PKIX_DECREF(verifyNode);
                        verifyNode = state->verifyNode;
                        state->verifyNode = NULL;
                        PKIX_DECREF(state);
                        state = parentState;
                        parentState = NULL;
                        if (state->verifyNode != NULL && verifyNode) {
                                PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                        (state->verifyNode,
                                        verifyNode,
                                        plContext),
                                        PKIX_VERIFYNODEADDTOTREEFAILED);
                                PKIX_DECREF(verifyNode);
                        }
                        PKIX_DECREF(validityDate);
                        PKIX_INCREF(state->validityDate);
                        validityDate = state->validityDate;
                        canBeCached = state->canBeCached;

                        
                        if (++(state->certIndex) < (state->numCerts)) {
                                state->status = BUILD_CERTVALIDATING;
                                PKIX_DECREF(state->candidateCert);
                                break;
                        }
                        if (state->useOnlyLocal == PKIX_TRUE) {
                            
                            pkix_PrepareForwardBuilderStateForAIA(state);
                            break;
                        }
                }
                PKIX_DECREF(state->candidateCert);
            } while (outOfOptions == PKIX_FALSE);

        } 

cleanup:

        if (pkixErrorClass == PKIX_FATAL_ERROR) {
            goto fatal;
        }

        



        PORT_Assert(verifyError == NULL);
        verifyError = pkixErrorResult;

        





        PKIX_CHECK_FATAL(pkix_ForwardBuilderState_IsIOPending
                         (state, &ioPending, plContext),
                PKIX_FORWARDBUILDERSTATEISIOPENDINGFAILED);

        if (ioPending == PKIX_FALSE) {
                while (state->parentState) {
                        PKIX_INCREF(state->parentState);
                        parentState = state->parentState;
                        PKIX_DECREF(verifyNode);
                        verifyNode = state->verifyNode;
                        state->verifyNode = NULL;
                        PKIX_DECREF(state);
                        state = parentState;
                        parentState = NULL;
                        if (state->verifyNode != NULL && verifyNode) {
                                PKIX_CHECK_FATAL(pkix_VerifyNode_AddToTree
                                        (state->verifyNode,
                                        verifyNode,
                                        plContext),
                                        PKIX_VERIFYNODEADDTOTREEFAILED);
                                PKIX_DECREF(verifyNode);
                        }
                }
                state->canBeCached = canBeCached;
                PKIX_DECREF(state->validityDate);
                state->validityDate = validityDate;
                validityDate = NULL;
        }
        if (!*pValResult && !verifyError) {
            if (!finalError) {
                PKIX_CHECK_FATAL(
                    pkix_VerifyNode_FindError(state->verifyNode,
                                              &finalError,
                                              plContext),
                    PKIX_VERIFYNODEFINDERRORFAILED);
            }
            if (finalError) {
                pkixErrorResult = finalError;
                pkixErrorCode = PKIX_BUILDFORWARDDEPTHFIRSTSEARCHFAILED;
                finalError = NULL;
                goto fatal;
            }
            pkixErrorCode = PKIX_SECERRORUNKNOWNISSUER;
            pkixErrorReceived = PKIX_TRUE;
            PKIX_ERROR_CREATE(BUILD, PKIX_SECERRORUNKNOWNISSUER,
                              verifyError);
            PKIX_CHECK_FATAL(
                pkix_VerifyNode_SetError(state->verifyNode, verifyError,
                                         plContext),
                PKIX_VERIFYNODESETERRORFAILED);
        } else {
            pkixErrorResult = verifyError;
            verifyError = NULL;
        }

fatal:
        if (state->parentState) {
            


            while (state->parentState) {
                PKIX_Error *error = NULL;
                PKIX_ForwardBuilderState *prntState = state->parentState;
                


                PKIX_INCREF(prntState);
                error = PKIX_PL_Object_DecRef((PKIX_PL_Object*)state, plContext);
                if (error) {
                    PKIX_PL_Object_DecRef((PKIX_PL_Object*)error, plContext);
                }
                

                state = prntState;
            }
        }
        PKIX_DECREF(parentState);
        PKIX_DECREF(childState);
        PKIX_DECREF(valResult);
        PKIX_DECREF(verifyError);
        PKIX_DECREF(finalError);
        PKIX_DECREF(verifyNode);
        PKIX_DECREF(childTraversedSubjNames);
        PKIX_DECREF(certSelParams);
        PKIX_DECREF(subjectNames);
        PKIX_DECREF(subjectName);
        PKIX_DECREF(trustAnchor);
        PKIX_DECREF(validityDate);
        PKIX_DECREF(revCheckerState);
        PKIX_DECREF(currTime);
        PKIX_DECREF(filteredCerts);
        PKIX_DECREF(unfilteredCerts);
        PKIX_DECREF(trustedCert);

        PKIX_RETURN(BUILD);
}



























static PKIX_Error*
pkix_Build_CheckInCache(
        PKIX_ForwardBuilderState *state,
        PKIX_BuildResult **pBuildResult,
        void **pNBIOContext,
        void *plContext)
{
        PKIX_PL_Cert *targetCert = NULL;
        PKIX_List *anchors = NULL;
        PKIX_PL_Date *testDate = NULL;
        PKIX_BuildResult *buildResult = NULL;
        PKIX_ValidateResult *valResult = NULL;
        PKIX_TrustAnchor *matchingAnchor = NULL;
        PKIX_PL_Cert *trustedCert = NULL;
        PKIX_List *certList = NULL;
        PKIX_Boolean cacheHit = PKIX_FALSE;
        PKIX_Boolean trusted = PKIX_FALSE;
        PKIX_Boolean stillValid = PKIX_FALSE;
        void *nbioContext = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_CheckInCache");

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        targetCert = state->buildConstants.targetCert;
        anchors = state->buildConstants.anchors;
        testDate = state->buildConstants.testDate;

        
        PKIX_CHECK(pkix_CacheCertChain_Lookup
                   (targetCert,
                    anchors,
                    testDate,
                    &cacheHit,
                    &buildResult,
                    plContext),
                   PKIX_CACHECERTCHAINLOOKUPFAILED);
        
        if (!cacheHit) {
            goto cleanup;
        }
        
        



        PKIX_CHECK(PKIX_BuildResult_GetValidateResult
                   (buildResult, &valResult, plContext),
                   PKIX_BUILDRESULTGETVALIDATERESULTFAILED);
        
        PKIX_CHECK(PKIX_ValidateResult_GetTrustAnchor
                       (valResult, &matchingAnchor, plContext),
                   PKIX_VALIDATERESULTGETTRUSTANCHORFAILED);
        
        PKIX_DECREF(valResult);
        
        PKIX_CHECK(PKIX_TrustAnchor_GetTrustedCert
                   (matchingAnchor, &trustedCert, plContext),
                   PKIX_TRUSTANCHORGETTRUSTEDCERTFAILED);
        
        if (state->buildConstants.anchors &&
            state->buildConstants.anchors->length) {
            
            PKIX_CHECK(
                pkix_List_Contains(state->buildConstants.anchors,
                                   (PKIX_PL_Object *)matchingAnchor,
                                   &trusted,
                                   plContext),
                PKIX_LISTCONTAINSFAILED);
        } else {
            PKIX_CHECK(PKIX_PL_Cert_IsCertTrusted
                       (trustedCert, PKIX_FALSE, &trusted, plContext),
                       PKIX_CERTISCERTTRUSTEDFAILED);
        }

        if (!trusted) {
            goto cleanup;
        }
        




        PKIX_CHECK(PKIX_BuildResult_GetCertChain
                   (buildResult, &certList, plContext),
                   PKIX_BUILDRESULTGETCERTCHAINFAILED);
        
        PKIX_CHECK(pkix_Build_ValidationCheckers
                   (state,
                    certList,
                    matchingAnchor,
                    PKIX_TRUE,  
                    plContext),
                   PKIX_BUILDVALIDATIONCHECKERSFAILED);

        PKIX_CHECK_ONLY_FATAL(
            pkix_Build_ValidateEntireChain(state, matchingAnchor,
                                           &nbioContext, &valResult,
                                           state->verifyNode, plContext),
            PKIX_BUILDVALIDATEENTIRECHAINFAILED);

        if (nbioContext != NULL) {
            
            *pNBIOContext = nbioContext;
            goto cleanup;
        }
        if (!PKIX_ERROR_RECEIVED) {
            
            *pBuildResult = buildResult;
            buildResult = NULL;
            stillValid = PKIX_TRUE;
        }

cleanup:

        if (!nbioContext && cacheHit && !(trusted && stillValid)) {
            


            PKIX_CHECK_FATAL(pkix_CacheCertChain_Remove
                       (targetCert,
                        anchors,
                        plContext),
                       PKIX_CACHECERTCHAINREMOVEFAILED);
        }

fatal:
       PKIX_DECREF(buildResult);
       PKIX_DECREF(valResult);
       PKIX_DECREF(certList);
       PKIX_DECREF(matchingAnchor);
       PKIX_DECREF(trustedCert);

       
       PKIX_RETURN(BUILD);
}















































static PKIX_Error *
pkix_Build_InitiateBuildChain(
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        PKIX_ForwardBuilderState **pState,
        PKIX_BuildResult **pBuildResult,
        PKIX_VerifyNode **pVerifyNode,
        void *plContext)
{
        PKIX_UInt32 numAnchors = 0;
        PKIX_UInt32 numCertStores = 0;
        PKIX_UInt32 numHintCerts = 0;
        PKIX_UInt32 i = 0;
        PKIX_Boolean isDuplicate = PKIX_FALSE;
        PKIX_PL_Cert *trustedCert = NULL;
        PKIX_CertSelector *targetConstraints = NULL;
        PKIX_ComCertSelParams *targetParams = NULL;
        PKIX_List *anchors = NULL;
        PKIX_List *targetSubjNames = NULL;
        PKIX_PL_Cert *targetCert = NULL;
        PKIX_PL_Object *firstHintCert = NULL;
        PKIX_RevocationChecker *revChecker = NULL;
        PKIX_List *certStores = NULL;
        PKIX_CertStore *certStore = NULL;
        PKIX_List *userCheckers = NULL;
        PKIX_List *hintCerts = NULL;
        PKIX_PL_Date *testDate = NULL;
        PKIX_PL_PublicKey *targetPubKey = NULL;
        void *nbioContext = NULL;
        BuildConstants buildConstants;

        PKIX_List *tentativeChain = NULL;
        PKIX_ValidateResult *valResult = NULL;
        PKIX_BuildResult *buildResult = NULL;
        PKIX_List *certList = NULL;
        PKIX_TrustAnchor *matchingAnchor = NULL;
        PKIX_ForwardBuilderState *state = NULL;
        PKIX_CertStore_CheckTrustCallback trustCallback = NULL;
        PKIX_CertSelector_MatchCallback selectorCallback = NULL;
        PKIX_PL_AIAMgr *aiaMgr = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_InitiateBuildChain");
        PKIX_NULLCHECK_FOUR(procParams, pNBIOContext, pState, pBuildResult);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        state = *pState;
        *pState = NULL; 

        if (state == NULL) {
            PKIX_CHECK(PKIX_ProcessingParams_GetDate
                    (procParams, &testDate, plContext),
                    PKIX_PROCESSINGPARAMSGETDATEFAILED);
    
            PKIX_CHECK(PKIX_ProcessingParams_GetTrustAnchors
                    (procParams, &anchors, plContext),
                    PKIX_PROCESSINGPARAMSGETTRUSTANCHORSFAILED);
    
            PKIX_CHECK(PKIX_List_GetLength(anchors, &numAnchors, plContext),
                    PKIX_LISTGETLENGTHFAILED);
    
            
            PKIX_CHECK(PKIX_ProcessingParams_GetTargetCertConstraints
                       (procParams, &targetConstraints, plContext),
                       PKIX_PROCESSINGPARAMSGETTARGETCERTCONSTRAINTSFAILED);
    
            PKIX_CHECK(PKIX_CertSelector_GetCommonCertSelectorParams
                    (targetConstraints, &targetParams, plContext),
                    PKIX_CERTSELECTORGETCOMMONCERTSELECTORPARAMSFAILED);
    
            PKIX_CHECK(PKIX_ComCertSelParams_GetCertificate
                    (targetParams, &targetCert, plContext),
                    PKIX_COMCERTSELPARAMSGETCERTIFICATEFAILED);
    
            PKIX_CHECK(
                PKIX_ComCertSelParams_SetLeafCertFlag(targetParams,
                                                      PKIX_TRUE, plContext),
                PKIX_COMCERTSELPARAMSSETLEAFCERTFLAGFAILED);

            PKIX_CHECK(PKIX_ProcessingParams_GetHintCerts
                        (procParams, &hintCerts, plContext),
                        PKIX_PROCESSINGPARAMSGETHINTCERTSFAILED);
    
            if (hintCerts != NULL) {
                    PKIX_CHECK(PKIX_List_GetLength
                            (hintCerts, &numHintCerts, plContext),
                            PKIX_LISTGETLENGTHFAILED);
            }

            





            if (targetCert == NULL) {

                    
                    if (numHintCerts == 0) {
                            PKIX_ERROR(PKIX_NOTARGETCERTSUPPLIED);
                    }

                    PKIX_CHECK(PKIX_List_GetItem
                            (hintCerts,
                            0,
                            (PKIX_PL_Object **)&targetCert,
                            plContext),
                            PKIX_LISTGETITEMFAILED);

                    PKIX_CHECK(PKIX_List_DeleteItem(hintCerts, 0, plContext),
                            PKIX_LISTGETITEMFAILED);
            } else {

                    


 
                    if (numHintCerts != 0) {
                            PKIX_CHECK(PKIX_List_GetItem
                                    (hintCerts, 0, &firstHintCert, plContext),
                                    PKIX_LISTGETITEMFAILED);

                            PKIX_CHECK(PKIX_PL_Object_Equals
                                    ((PKIX_PL_Object *)targetCert,
                                    firstHintCert,
                                    &isDuplicate,
                                    plContext),
                                    PKIX_OBJECTEQUALSFAILED);

                            if (isDuplicate) {
                                    PKIX_CHECK(PKIX_List_DeleteItem
                                    (hintCerts, 0, plContext),
                                    PKIX_LISTGETITEMFAILED);
                            }
                            PKIX_DECREF(firstHintCert);
                    }

            }

            if (targetCert == NULL) {
                PKIX_ERROR(PKIX_NOTARGETCERTSUPPLIED);
            }

            PKIX_CHECK(PKIX_PL_Cert_GetAllSubjectNames
                    (targetCert,
                    &targetSubjNames,
                    plContext),
                    PKIX_CERTGETALLSUBJECTNAMESFAILED);
    
            PKIX_CHECK(PKIX_PL_Cert_GetSubjectPublicKey
                    (targetCert, &targetPubKey, plContext),
                    PKIX_CERTGETSUBJECTPUBLICKEYFAILED);
    
            PKIX_CHECK(PKIX_List_Create(&tentativeChain, plContext),
                    PKIX_LISTCREATEFAILED);
    
            PKIX_CHECK(PKIX_List_AppendItem
                    (tentativeChain, (PKIX_PL_Object *)targetCert, plContext),
                    PKIX_LISTAPPENDITEMFAILED);
    
            if (procParams->qualifyTargetCert) {
                
                
                PKIX_CHECK(
                    PKIX_ComCertSelParams_SetCertificateValid(targetParams,
                                                              testDate,
                                                              plContext),
                    PKIX_COMCERTSELPARAMSSETCERTIFICATEVALIDFAILED);
                
                PKIX_CHECK(PKIX_CertSelector_GetMatchCallback
                           (targetConstraints, &selectorCallback, plContext),
                           PKIX_CERTSELECTORGETMATCHCALLBACKFAILED);
                
                pkixErrorResult =
                    (*selectorCallback)(targetConstraints, targetCert,
                                        plContext);
                if (pkixErrorResult) {
                    pkixErrorClass = pkixErrorResult->errClass;
                    if (pkixErrorClass == PKIX_FATAL_ERROR) {
                        goto cleanup;
                    }
                    if (pVerifyNode != NULL) {
                            PKIX_Error *tempResult =
                                pkix_VerifyNode_Create(targetCert, 0,
                                                       pkixErrorResult,
                                                       pVerifyNode,
                                                       plContext);
                            if (tempResult) {
                                PKIX_DECREF(pkixErrorResult);
                                pkixErrorResult = tempResult;
                                pkixErrorCode = PKIX_VERIFYNODECREATEFAILED;
                                pkixErrorClass = PKIX_FATAL_ERROR;
                                goto cleanup;
                            }
                    }
                    pkixErrorCode = PKIX_CERTCHECKVALIDITYFAILED;
                    goto cleanup;
                }
            }
    
            PKIX_CHECK(PKIX_ProcessingParams_GetCertStores
                    (procParams, &certStores, plContext),
                    PKIX_PROCESSINGPARAMSGETCERTSTORESFAILED);
    
            PKIX_CHECK(PKIX_List_GetLength
                    (certStores, &numCertStores, plContext),
                    PKIX_LISTGETLENGTHFAILED);
    
            
            if (numCertStores > 1) {
                for (i = numCertStores - 1; i > 0; i--) {
                    PKIX_CHECK_ONLY_FATAL(PKIX_List_GetItem
                        (certStores,
                        i,
                        (PKIX_PL_Object **)&certStore,
                        plContext),
                        PKIX_LISTGETITEMFAILED);
                    PKIX_CHECK_ONLY_FATAL(PKIX_CertStore_GetTrustCallback
                        (certStore, &trustCallback, plContext),
                        PKIX_CERTSTOREGETTRUSTCALLBACKFAILED);
    
                    if (trustCallback != NULL) {
                        
                        PKIX_CHECK(PKIX_List_DeleteItem
                            (certStores, i, plContext),
                            PKIX_LISTDELETEITEMFAILED);
                        PKIX_CHECK(PKIX_List_InsertItem
                            (certStores,
                            0,
                            (PKIX_PL_Object *)certStore,
                            plContext),
                        PKIX_LISTINSERTITEMFAILED);
    
                    }
    
                    PKIX_DECREF(certStore);
                }
            }
    
            PKIX_CHECK(PKIX_ProcessingParams_GetCertChainCheckers
                        (procParams, &userCheckers, plContext),
                        PKIX_PROCESSINGPARAMSGETCERTCHAINCHECKERSFAILED);
    
            PKIX_CHECK(PKIX_ProcessingParams_GetRevocationChecker
                        (procParams, &revChecker, plContext),
                       PKIX_PROCESSINGPARAMSGETREVOCATIONCHECKERFAILED);
            

            if (procParams->useAIAForCertFetching) {
                PKIX_CHECK(PKIX_PL_AIAMgr_Create(&aiaMgr, plContext),
                           PKIX_AIAMGRCREATEFAILED);
            }

            



    
            buildConstants.numAnchors = numAnchors;
            buildConstants.numCertStores = numCertStores;
            buildConstants.numHintCerts = numHintCerts;
            buildConstants.procParams = procParams;
            buildConstants.testDate = testDate;
            buildConstants.timeLimit = NULL;
            buildConstants.targetCert = targetCert;
            buildConstants.targetPubKey = targetPubKey;
            buildConstants.certStores = certStores;
            buildConstants.anchors = anchors;
            buildConstants.userCheckers = userCheckers;
            buildConstants.hintCerts = hintCerts;
            buildConstants.revChecker = revChecker;
            buildConstants.aiaMgr = aiaMgr;
                
            PKIX_CHECK(pkix_Build_GetResourceLimits(&buildConstants, plContext),
                    PKIX_BUILDGETRESOURCELIMITSFAILED);
    
            PKIX_CHECK(pkix_ForwardBuilderState_Create
                    (0,              
                    buildConstants.maxFanout,
                    buildConstants.maxDepth,
                    PKIX_FALSE,      
                    PKIX_TRUE,       
                    NULL,            
                    targetCert,      
                    targetSubjNames, 
                    tentativeChain,  
                    NULL,            
                    &state,          
                    plContext),
                    PKIX_BUILDSTATECREATEFAILED);
    
            state->buildConstants.numAnchors = buildConstants.numAnchors;
            state->buildConstants.numCertStores = buildConstants.numCertStores; 
            state->buildConstants.numHintCerts = buildConstants.numHintCerts;
            state->buildConstants.maxFanout = buildConstants.maxFanout;
            state->buildConstants.maxDepth = buildConstants.maxDepth;
            state->buildConstants.maxTime = buildConstants.maxTime;
            state->buildConstants.procParams = buildConstants.procParams; 
            PKIX_INCREF(buildConstants.testDate);
            state->buildConstants.testDate = buildConstants.testDate;
            state->buildConstants.timeLimit = buildConstants.timeLimit;
            PKIX_INCREF(buildConstants.targetCert);
            state->buildConstants.targetCert = buildConstants.targetCert;
            PKIX_INCREF(buildConstants.targetPubKey);
            state->buildConstants.targetPubKey =
                    buildConstants.targetPubKey;
            PKIX_INCREF(buildConstants.certStores);
            state->buildConstants.certStores = buildConstants.certStores;
            PKIX_INCREF(buildConstants.anchors);
            state->buildConstants.anchors = buildConstants.anchors;
            PKIX_INCREF(buildConstants.userCheckers);
            state->buildConstants.userCheckers =
                    buildConstants.userCheckers;
            PKIX_INCREF(buildConstants.hintCerts);
            state->buildConstants.hintCerts = buildConstants.hintCerts;
            PKIX_INCREF(buildConstants.revChecker);
            state->buildConstants.revChecker = buildConstants.revChecker;
            state->buildConstants.aiaMgr = buildConstants.aiaMgr;
            aiaMgr = NULL;

            if (buildConstants.maxTime != 0) {
                    PKIX_CHECK(PKIX_PL_Date_Create_CurrentOffBySeconds
                            (buildConstants.maxTime,
                            &state->buildConstants.timeLimit,
                            plContext),
                            PKIX_DATECREATECURRENTOFFBYSECONDSFAILED);
            }

            if (pVerifyNode != NULL) {
                PKIX_Error *tempResult =
                    pkix_VerifyNode_Create(targetCert, 0, NULL,
                                           &(state->verifyNode),
                                           plContext);
                if (tempResult) {
                    pkixErrorResult = tempResult;
                    pkixErrorCode = PKIX_VERIFYNODECREATEFAILED;
                    pkixErrorClass = PKIX_FATAL_ERROR;
                    goto cleanup;
                }
            }

            PKIX_CHECK_ONLY_FATAL(
                pkix_Build_CheckInCache(state, &buildResult,
                                        &nbioContext, plContext),
                PKIX_UNABLETOBUILDCHAIN);
            if (nbioContext) {
                *pNBIOContext = nbioContext;
                *pState = state;
                state = NULL;
                goto cleanup;
            }
            if (buildResult) {
                *pBuildResult = buildResult;
                if (pVerifyNode != NULL) {
                    *pVerifyNode = state->verifyNode;
                    state->verifyNode = NULL;
                }
                goto cleanup;
            }
        }

        
        if (targetSubjNames == NULL) {
            PKIX_CHECK(PKIX_PL_Cert_GetAllSubjectNames
                    (state->buildConstants.targetCert,
                    &targetSubjNames,
                    plContext),
                    PKIX_CERTGETALLSUBJECTNAMESFAILED);
        }

        state->status = BUILD_INITIAL;

        if (!matchingAnchor) {
                pkixErrorResult =
                    pkix_BuildForwardDepthFirstSearch(&nbioContext, state,
                                                      &valResult, plContext);
        }

        
        if (pkixErrorResult == NULL && nbioContext != NULL) {

                *pNBIOContext = nbioContext;
                *pBuildResult = NULL;

        
        } else {
                if (pVerifyNode != NULL) {
                        PKIX_INCREF(state->verifyNode);
                        *pVerifyNode = state->verifyNode;
                }

                if (valResult == NULL || pkixErrorResult)
                        PKIX_ERROR(PKIX_UNABLETOBUILDCHAIN);
                PKIX_CHECK(
                    pkix_BuildResult_Create(valResult, state->trustChain,
                                            &buildResult, plContext),
                    PKIX_BUILDRESULTCREATEFAILED);
                *pBuildResult = buildResult;
        }

        *pState = state;
        state = NULL;

cleanup:

        PKIX_DECREF(targetConstraints);
        PKIX_DECREF(targetParams);
        PKIX_DECREF(anchors);
        PKIX_DECREF(targetSubjNames);
        PKIX_DECREF(targetCert);
        PKIX_DECREF(revChecker);
        PKIX_DECREF(certStores);
        PKIX_DECREF(certStore);
        PKIX_DECREF(userCheckers);
        PKIX_DECREF(hintCerts);
        PKIX_DECREF(firstHintCert);
        PKIX_DECREF(testDate);
        PKIX_DECREF(targetPubKey);
        PKIX_DECREF(tentativeChain);
        PKIX_DECREF(valResult);
        PKIX_DECREF(certList);
        PKIX_DECREF(matchingAnchor);
        PKIX_DECREF(trustedCert);
        PKIX_DECREF(state);
        PKIX_DECREF(aiaMgr);

        PKIX_RETURN(BUILD);
}




































static PKIX_Error *
pkix_Build_ResumeBuildChain(
        void **pNBIOContext,
        PKIX_ForwardBuilderState *state,
        PKIX_BuildResult **pBuildResult,
        PKIX_VerifyNode **pVerifyNode,
        void *plContext)
{
        PKIX_ValidateResult *valResult = NULL;
        PKIX_BuildResult *buildResult = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(BUILD, "pkix_Build_ResumeBuildChain");
        PKIX_NULLCHECK_TWO(state, pBuildResult);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        pkixErrorResult =
            pkix_BuildForwardDepthFirstSearch(&nbioContext, state,
                                              &valResult, plContext);

        
        if (pkixErrorResult == NULL && nbioContext != NULL) {

                *pNBIOContext = nbioContext;
                *pBuildResult = NULL;

        
        } else {
                if (pVerifyNode != NULL) {
                    PKIX_INCREF(state->verifyNode);
                    *pVerifyNode = state->verifyNode;
                }

                if (valResult == NULL || pkixErrorResult)
                    PKIX_ERROR(PKIX_UNABLETOBUILDCHAIN);

                PKIX_CHECK(
                    pkix_BuildResult_Create(valResult, state->trustChain,
                                            &buildResult, plContext),
                    PKIX_BUILDRESULTCREATEFAILED);
                *pBuildResult = buildResult;
        }

cleanup:

        PKIX_DECREF(valResult);

        PKIX_RETURN(BUILD);
}






PKIX_Error *
PKIX_BuildChain(
        PKIX_ProcessingParams *procParams,
        void **pNBIOContext,
        void **pState,
        PKIX_BuildResult **pBuildResult,
        PKIX_VerifyNode **pVerifyNode,
        void *plContext)
{
        PKIX_ForwardBuilderState *state = NULL;
        PKIX_BuildResult *buildResult = NULL;
        void *nbioContext = NULL;

        PKIX_ENTER(BUILD, "PKIX_BuildChain");
        PKIX_NULLCHECK_FOUR(procParams, pNBIOContext, pState, pBuildResult);

        nbioContext = *pNBIOContext;
        *pNBIOContext = NULL;

        if (*pState == NULL) {
                PKIX_CHECK(pkix_Build_InitiateBuildChain
                        (procParams,
                        &nbioContext,
                        &state,
                        &buildResult,
                        pVerifyNode,
                        plContext),
                        PKIX_BUILDINITIATEBUILDCHAINFAILED);
        } else {
                state = (PKIX_ForwardBuilderState *)(*pState);
                *pState = NULL; 
                if (state->status == BUILD_SHORTCUTPENDING) {
                        PKIX_CHECK(pkix_Build_InitiateBuildChain
                                (procParams,
                                &nbioContext,
                                &state,
                                &buildResult,
                                pVerifyNode,
                                plContext),
                                PKIX_BUILDINITIATEBUILDCHAINFAILED);
                } else {
                        PKIX_CHECK(pkix_Build_ResumeBuildChain
                                (&nbioContext,
                                state,
                                &buildResult,
                                pVerifyNode,
                                plContext),
                                PKIX_BUILDINITIATEBUILDCHAINFAILED);
                }
        }

        
        if (nbioContext != NULL) {

                *pNBIOContext = nbioContext;
                *pState = state;
                state = NULL;
                *pBuildResult = NULL;

        
        } else if (buildResult == NULL) {
                PKIX_ERROR(PKIX_UNABLETOBUILDCHAIN);
        } else {
                





                if ((state != NULL) &&
                    ((state->validityDate) != NULL) &&
                    (state->canBeCached)) {
                        PKIX_CHECK(pkix_CacheCertChain_Add
                                (state->buildConstants.targetCert,
                                state->buildConstants.anchors,
                                state->validityDate,
                                buildResult,
                                plContext),
                                PKIX_CACHECERTCHAINADDFAILED);
                }

                *pState = NULL;
                *pBuildResult = buildResult;
                buildResult = NULL;
        }

cleanup:
        PKIX_DECREF(buildResult);
        PKIX_DECREF(state);

        PKIX_RETURN(BUILD);
}
