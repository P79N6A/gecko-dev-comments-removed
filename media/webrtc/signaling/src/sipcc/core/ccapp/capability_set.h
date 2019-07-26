



#ifndef _CC_CAPABILITY_SET_H_
#define _CC_CAPABILITY_SET_H_

#include "ccapi_types.h"

extern void capset_get_idleset( cc_cucm_mode_t mode, cc_boolean features[]);
extern void capset_get_allowed_features( cc_cucm_mode_t mode, cc_call_state_t state, cc_boolean features[]);


int  fcp_init_template (const char* fcp_plan_string);


#define FCP_MAX_SIZE                0x5000
#define FCP_FEATURE_NAME_MAX        24
#define MAX_FP_VERSION_STAMP_LEN   (64+1)

extern char g_fp_version_stamp[MAX_FP_VERSION_STAMP_LEN];




typedef struct cc_feature_control_policy_info_t_ 
{
    char                         featureName[FCP_FEATURE_NAME_MAX];
    unsigned int                 featureId;
    cc_boolean                   featureEnabled;
} cc_feature_control_policy_info_t;

#endif 
