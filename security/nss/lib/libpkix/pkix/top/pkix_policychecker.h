










































#ifndef _PKIX_POLICYCHECKER_H
#define _PKIX_POLICYCHECKER_H

#include "pkix_tools.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PKIX_PolicyCheckerStateStruct PKIX_PolicyCheckerState;

struct PKIX_PolicyCheckerStateStruct{
        PKIX_PL_OID *certPoliciesExtension;             
        PKIX_PL_OID *policyMappingsExtension;           
        PKIX_PL_OID *policyConstraintsExtension;        
        PKIX_PL_OID *inhibitAnyPolicyExtension;         
        PKIX_PL_OID *anyPolicyOID;                      
        PKIX_Boolean initialIsAnyPolicy;                
        PKIX_PolicyNode *validPolicyTree;
        PKIX_List *userInitialPolicySet;                
        PKIX_List *mappedUserInitialPolicySet;
        PKIX_Boolean policyQualifiersRejected;
        PKIX_Boolean initialPolicyMappingInhibit;
        PKIX_Boolean initialExplicitPolicy;
        PKIX_Boolean initialAnyPolicyInhibit;
        PKIX_UInt32 explicitPolicy;
        PKIX_UInt32 inhibitAnyPolicy;
        PKIX_UInt32 policyMapping;
        PKIX_UInt32 numCerts;
        PKIX_UInt32 certsProcessed;
        PKIX_PolicyNode *anyPolicyNodeAtBottom;
        PKIX_PolicyNode *newAnyPolicyNode;
        





        PKIX_Boolean certPoliciesCritical;
        PKIX_List *mappedPolicyOIDs;
};

PKIX_Error *
pkix_PolicyChecker_Initialize(
        PKIX_List *initialPolicies,
        PKIX_Boolean policyQualifiersRejected,
        PKIX_Boolean initialPolicyMappingInhibit,
        PKIX_Boolean initialExplicitPolicy,
        PKIX_Boolean initialAnyPolicyInhibit,
        PKIX_UInt32 numCerts,
        PKIX_CertChainChecker **pChecker,
        void *plContext);



PKIX_Error *
pkix_PolicyCheckerState_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
