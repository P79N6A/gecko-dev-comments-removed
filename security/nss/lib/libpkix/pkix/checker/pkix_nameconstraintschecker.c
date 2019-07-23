










































#include "pkix_nameconstraintschecker.h"







static PKIX_Error *
pkix_NameConstraintsCheckerState_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        pkix_NameConstraintsCheckerState *state = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTSCHECKERSTATE,
                    "pkix_NameConstraintsCheckerState_Destroy");
        PKIX_NULLCHECK_ONE(object);

        
        PKIX_CHECK(pkix_CheckType
            (object, PKIX_CERTNAMECONSTRAINTSCHECKERSTATE_TYPE, plContext),
            PKIX_OBJECTNOTNAMECONSTRAINTSCHECKERSTATE);

        state = (pkix_NameConstraintsCheckerState *)object;

        PKIX_DECREF(state->nameConstraints);
        PKIX_DECREF(state->nameConstraintsOID);

cleanup:

        PKIX_RETURN(CERTNAMECONSTRAINTSCHECKERSTATE);
}















PKIX_Error *
pkix_NameConstraintsCheckerState_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(CERTNAMECONSTRAINTSCHECKERSTATE,
                    "pkix_NameConstraintsCheckerState_RegisterSelf");

        entry.description = "NameConstraintsCheckerState";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(pkix_NameConstraintsCheckerState);
        entry.destructor = pkix_NameConstraintsCheckerState_Destroy;
        entry.equalsFunction = NULL;
        entry.hashcodeFunction = NULL;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_CERTNAMECONSTRAINTSCHECKERSTATE_TYPE] = entry;

        PKIX_RETURN(CERTNAMECONSTRAINTSCHECKERSTATE);
}



























static PKIX_Error *
pkix_NameConstraintsCheckerState_Create(
    PKIX_PL_CertNameConstraints *nameConstraints,
    PKIX_UInt32 numCerts,
    pkix_NameConstraintsCheckerState **pCheckerState,
    void *plContext)
{
        pkix_NameConstraintsCheckerState *state = NULL;

        PKIX_ENTER(CERTNAMECONSTRAINTSCHECKERSTATE,
                    "pkix_NameConstraintsCheckerState_Create");
        PKIX_NULLCHECK_ONE(pCheckerState);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_CERTNAMECONSTRAINTSCHECKERSTATE_TYPE,
                    sizeof (pkix_NameConstraintsCheckerState),
                    (PKIX_PL_Object **)&state,
                    plContext),
                    PKIX_COULDNOTCREATENAMECONSTRAINTSCHECKERSTATEOBJECT);

        

        PKIX_CHECK(PKIX_PL_OID_Create
                    (PKIX_NAMECONSTRAINTS_OID,
                    &state->nameConstraintsOID,
                    plContext),
                    PKIX_OIDCREATEFAILED);

        PKIX_INCREF(nameConstraints);

        state->nameConstraints = nameConstraints;
        state->certsRemaining = numCerts;

        *pCheckerState = state;
        state = NULL;

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(CERTNAMECONSTRAINTSCHECKERSTATE);
}







static PKIX_Error *
pkix_NameConstraintsChecker_Check(
        PKIX_CertChainChecker *checker,
        PKIX_PL_Cert *cert,
        PKIX_List *unresolvedCriticalExtensions,
        void **pNBIOContext,
        void *plContext)
{
        pkix_NameConstraintsCheckerState *state = NULL;
        PKIX_PL_CertNameConstraints *nameConstraints = NULL;
        PKIX_PL_CertNameConstraints *mergedNameConstraints = NULL;
        PKIX_Boolean selfIssued = PKIX_FALSE;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_NameConstraintsChecker_Check");
        PKIX_NULLCHECK_THREE(checker, cert, pNBIOContext);

        *pNBIOContext = NULL; 

        PKIX_CHECK(PKIX_CertChainChecker_GetCertChainCheckerState
                    (checker, (PKIX_PL_Object **)&state, plContext),
                    PKIX_CERTCHAINCHECKERGETCERTCHAINCHECKERSTATEFAILED);

        state->certsRemaining--;

        
        PKIX_CHECK(pkix_IsCertSelfIssued(cert, &selfIssued, plContext),
                    PKIX_ISCERTSELFISSUEDFAILED);

        
        if (selfIssued == PKIX_FALSE ||
            (selfIssued == PKIX_TRUE && state->certsRemaining == 0)) {
                PKIX_CHECK(PKIX_PL_Cert_CheckNameConstraints
                    (cert, state->nameConstraints, plContext),
                    PKIX_CERTCHECKNAMECONSTRAINTSFAILED);
        }

        if (state->certsRemaining != 0) {

            PKIX_CHECK(PKIX_PL_Cert_GetNameConstraints
                    (cert, &nameConstraints, plContext),
                    PKIX_CERTGETNAMECONSTRAINTSFAILED);

            

            if (nameConstraints != NULL) {

                if (state->nameConstraints == NULL) {

                        state->nameConstraints = nameConstraints;

                } else {

                        PKIX_CHECK(PKIX_PL_Cert_MergeNameConstraints
                                (nameConstraints,
                                state->nameConstraints,
                                &mergedNameConstraints,
                                plContext),
                                PKIX_CERTMERGENAMECONSTRAINTSFAILED);

                        PKIX_DECREF(nameConstraints);
                        PKIX_DECREF(state->nameConstraints);

                        state->nameConstraints = mergedNameConstraints;
                }

                
                if (unresolvedCriticalExtensions != NULL) {
                        PKIX_CHECK(pkix_List_Remove
                                    (unresolvedCriticalExtensions,
                                    (PKIX_PL_Object *)state->nameConstraintsOID,
                                    plContext),
                                    PKIX_LISTREMOVEFAILED);
                }
            }
        }

        PKIX_CHECK(PKIX_CertChainChecker_SetCertChainCheckerState
                    (checker, (PKIX_PL_Object *)state, plContext),
                    PKIX_CERTCHAINCHECKERSETCERTCHAINCHECKERSTATEFAILED);

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(CERTCHAINCHECKER);
}






























PKIX_Error *
pkix_NameConstraintsChecker_Initialize(
        PKIX_PL_CertNameConstraints *trustedNC,
        PKIX_UInt32 numCerts,
        PKIX_CertChainChecker **pChecker,
        void *plContext)
{
        pkix_NameConstraintsCheckerState *state = NULL;

        PKIX_ENTER(CERTCHAINCHECKER, "pkix_NameConstraintsChecker_Initialize");
        PKIX_NULLCHECK_ONE(pChecker);

        PKIX_CHECK(pkix_NameConstraintsCheckerState_Create
                    (trustedNC, numCerts, &state, plContext),
                    PKIX_NAMECONSTRAINTSCHECKERSTATECREATEFAILED);

        PKIX_CHECK(PKIX_CertChainChecker_Create
                    (pkix_NameConstraintsChecker_Check,
                    PKIX_FALSE,
                    PKIX_FALSE,
                    NULL,
                    (PKIX_PL_Object *) state,
                    pChecker,
                    plContext),
                    PKIX_CERTCHAINCHECKERCREATEFAILED);

cleanup:

        PKIX_DECREF(state);

        PKIX_RETURN(CERTCHAINCHECKER);
}
