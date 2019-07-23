










































#ifndef _PKIX_REVOCATIONCHECKER_H
#define _PKIX_REVOCATIONCHECKER_H

#include "pkixt.h"

#ifdef __cplusplus
extern "C" {
#endif












#define PKIX_REV_M_DO_NOT_TEST_USING_THIS_METHOD     0x00L
#define PKIX_REV_M_TEST_USING_THIS_METHOD            0x01L







#define PKIX_REV_M_ALLOW_NETWORK_FETCHING            0x00L
#define PKIX_REV_M_FORBID_NETWORK_FETCHING           0x02L












 
#define PKIX_REV_M_ALLOW_IMPLICIT_DEFAULT_SOURCE     0x00L
#define PKIX_REV_M_IGNORE_IMPLICIT_DEFAULT_SOURCE    0x04L /* OCSP only */














#define PKIX_REV_M_SKIP_TEST_ON_MISSING_SOURCE       0x00L
#define PKIX_REV_M_REQUIRE_INFO_ON_MISSING_SOURCE    0x08L









#define PKIX_REV_M_IGNORE_MISSING_FRESH_INFO         0x00L
#define PKIX_REV_M_FAIL_ON_MISSING_FRESH_INFO        0x10L












#define PKIX_REV_M_STOP_TESTING_ON_FRESH_INFO        0x00L
#define PKIX_REV_M_CONTINUE_TESTING_ON_FRESH_INFO    0x20L


















#define PKIX_REV_MI_TEST_EACH_METHOD_SEPARATELY       0x00L
#define PKIX_REV_MI_TEST_ALL_LOCAL_INFORMATION_FIRST  0x01L












#define PKIX_REV_MI_NO_OVERALL_INFO_REQUIREMENT       0x00L
#define PKIX_REV_MI_REQUIRE_SOME_FRESH_INFO_AVAILABLE 0x02L



struct PKIX_RevocationCheckerStruct {
    PKIX_PL_Date *date;
    PKIX_List *leafMethodList;
    PKIX_List *chainMethodList;
    PKIX_UInt32 leafMethodListFlags;
    PKIX_UInt32 chainMethodListFlags;
};



PKIX_Error *pkix_RevocationChecker_RegisterSelf(void *plContext);

#ifdef __cplusplus
}
#endif

#endif
