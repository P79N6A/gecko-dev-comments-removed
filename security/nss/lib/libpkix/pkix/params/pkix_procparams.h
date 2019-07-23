










































#ifndef _PKIX_PROCESSINGPARAMS_H
#define _PKIX_PROCESSINGPARAMS_H

#include "pkix_tools.h"


#ifdef __cplusplus
extern "C" {
#endif

struct PKIX_ProcessingParamsStruct {
        PKIX_List *trustAnchors;        
        PKIX_List *hintCerts;	
        PKIX_CertSelector *constraints;
        PKIX_PL_Date *date;
        PKIX_List *initialPolicies;     
        PKIX_Boolean initialPolicyMappingInhibit;
        PKIX_Boolean initialAnyPolicyInhibit;
        PKIX_Boolean initialExplicitPolicy;
        PKIX_Boolean qualifiersRejected;
        PKIX_List *certChainCheckers;
        PKIX_List *certStores;
        PKIX_Boolean isCrlRevocationCheckingEnabled;
        PKIX_Boolean isCrlRevocationCheckingEnabledWithNISTPolicy;
        PKIX_RevocationChecker *revChecker;
        PKIX_ResourceLimits *resourceLimits;
        PKIX_Boolean useAIAForCertFetching;
        PKIX_Boolean qualifyTargetCert;
};



PKIX_Error *pkix_ProcessingParams_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
