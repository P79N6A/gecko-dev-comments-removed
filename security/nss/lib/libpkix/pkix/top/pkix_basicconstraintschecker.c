










































#include "pkix_basicconstraintschecker.h"







static PKIX_Error *
pkix_BasicConstraintsCheckerState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_BasicConstraintsCheckerState *state = NULL;

        PKIX_ENTER(BASICCONSTRAINTSCHECKERSTATE,
                    "pkix_BasicConstraintsCheckerState_Destroy");

        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
                (object, PKIX_BASICCONSTRAINTSCHECKERSTATE_TYPE, plContext),
                PKIX_OBJECTNOTBASICCONSTRAINTSCHECKERSTATE);

        state = (pkix_BasicConstraintsCheckerState *)object;

        PKIX_DECREF(state->basicConstraintsOID);

cleanup:

        PKIX_RETURN(BASICCONSTRAINTSCHECKERSTATE);
}












PKIX_Error *
pkix_BasicConstraintsCheckerState_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(BASICCONSTRAINTSCHECKERSTATE,
                "pkix_BasicConstraintsCheckerState_RegisterSelf");

        entry.description = "BasicConstraintsCheckerState";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(pkix_BasicConstraintsCheckerState);
        entry.destructor = pkix_BasicConstraintsCheckerState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_BASICCONSTRAINTSCHECKERSTATE_TYPE] = entry;

        PKIX_RETURN(BASICCONSTRAINTSCHECKERSTATE);
}























static PKIX_Error *
pkix_BasicConstraintsCheckerState_Create(
        PKIX_UInt32 certsRemaining,
        pkix_BasicConstraintsCheckerState **pState,
        void *plContext)
{
        pkix_BasicConstraintsCheckerState *state = NULL;

        PKIX_ENTER(BASICCONSTRAINTSCHECKERSTATE,
                    "pkix_BasicConstraintsCheckerState_Create");

        PKIX_NULLCHECK_ONE(pState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_BASICCONSTRAINTSCHECKERSTATE_TYPE,
                    sizeof (pkix_BasicConstraintsCheckerState),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    PKIX_COULDNOTCREATEBASICCONSTRAINTSSTATEOBJECT);

        
        state->certsRemaining = certsRemaining;
        state->maxPathLength = PKIX_UNLIMITED_PATH_CONSTRAINT;

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_BASICCONSTRAINTS_OID,
                    &state->basicConstraintsOID,
                    plContext),
                    PKIX_OIDCREATEFAILED);

        *pState = state;
        state = NULL;

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(BASICCONSTRAINTSCHECKERSTATE);
}







PKIX_Error *
pkix_BasicConstraintsChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,  
        void **pNBIOContext,
        void *plContext)
{
        PKIX_PL_CertBasicConstraints *basicConstraints = NULL;
        pkix_BasicConstraintsCheckerState *state = NULL;
        PKIX_Boolean caFlag = PKIX_FALSE;
        PKIX_Int32 pathLength = 0;
        PKIX_Int32 maxPathLength_now;
        PKIX_Boolean isSelfIssued = PKIX_FALSE;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_BasicConstraintsChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        state->certsRemaining--;

        if (state->certsRemaining != 0) {

                PKIX_CHECK(PKIX_PL_Cert_GetBasicConstraints
                    (cert, &basicConstraints, plContext),
                    PKIX_CERTGETBASICCONSTRAINTSFAILED);

                
                if (basicConstraints != NULL) {
                        PKIX_CHECK(PKIX_PL_BasicConstraints_GetCAFlag
                            (basicConstraints,
                            &caFlag,
                            plContext),
                            PKIX_BASICCONSTRAINTSGETCAFLAGFAILED);

                if (caFlag == PKIX_TRUE) {
                        PKIX_CHECK
                            (PKIX_PL_BasicConstraints_GetPathLenConstraint
                            (basicConstraints,
                            &pathLength,
                            plContext),
                            PKIX_BASICCONSTRAINTSGETPATHLENCONSTRAINTFAILED);
                }

                }else{
                        caFlag = PKIX_FALSE;
                        pathLength = PKIX_UNLIMITED_PATH_CONSTRAINT;
                }

                PKIX_CHECK(pkix_IsCertSelfIssued
                        (cert,
                        &isSelfIssued,
                        plContext),
                        PKIX_ISCERTSELFISSUEDFAILED);

                maxPathLength_now = state->maxPathLength;

                if (isSelfIssued != PKIX_TRUE) {

                    
                    if (maxPathLength_now == 0) {
                        PKIX_ERROR(PKIX_BASICCONSTRAINTSVALIDATIONFAILEDLN);
                    }

                    if (caFlag == PKIX_FALSE) {
                        PKIX_ERROR(PKIX_BASICCONSTRAINTSVALIDATIONFAILEDCA);
                    }

                    if (maxPathLength_now > 0) { 
                        maxPathLength_now--;
                    }

                }

                if (caFlag == PKIX_TRUE) {
                    if (maxPathLength_now == PKIX_UNLIMITED_PATH_CONSTRAINT){
                            maxPathLength_now = pathLength;
                    } else {
                            
                        if (pathLength != PKIX_UNLIMITED_PATH_CONSTRAINT) {
                            maxPathLength_now =
                                    (maxPathLength_now > pathLength)?
                                    pathLength:maxPathLength_now;
                        }
                    }
                }

                state->maxPathLength = maxPathLength_now;
        }

        
        if (unresolvedCriticalExtensions != NULL) {

                PKIX_CHECK(pkix_List_Remove
                            (unresolvedCriticalExtensions,
                            (PKIX_PL_Object *) state->basicConstraintsOID,
                            plContext),
                            PKIX_LISTREMOVEFAILED);
        }


        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                    (checker, (PKIX_PL_Object *)state, plContext),
                    PKIX_CERTCHAINCHECKERSETCERTCHAINCHECKERSTATEFAILED);


cleanup:
        PKIX_DECREF(state);
        PKIX_DECREF(basicConstraints);
        PKIX_RETURN(CERTCHAINCHECKER);

}












PKIX_Error *
pkix_BasicConstraintsChecker_Initialize(
        PKIX_UInt32 certsRemaining,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        pkix_BasicConstraintsCheckerState *state = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_BasicConstraintsChecker_Initialize");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(pkix_BasicConstraintsCheckerState_Create
                    (certsRemaining, &state, plContext),
                    PKIX_BASICCONSTRAINTSCHECKERSTATECREATEFAILED);

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_BasicConstraintsChecker_Check,
                    PKIX_FALSE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *)state,
                    pChecker,
                    plContext),
                    PKIX_CERTCHAINCHECKERCHECKFAILED);

cleanup:
        PKIX_DECREF(state);

        PKIX_RETURN(CERTCHAINCHECKER);
}
