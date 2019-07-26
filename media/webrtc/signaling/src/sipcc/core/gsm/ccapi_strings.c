






































#include "cpr_types.h"
#define __CC_FEATURE_STRINGS__
#define __CC_MESSAGES_STRINGS__
#define __CC_CAUSE_STRINGS__
#include "text_strings.h"
#include "ccapi.h"








const char *cc_feature_name (cc_features_t id)
{
    if ((id <= CC_FEATURE_MIN) || (id >= CC_FEATURE_MAX)) {
        return get_debug_string(GSM_UNDEFINED);
    }

    return cc_feature_names[id - CC_FEATURE_NONE];
}









const char *cc_msg_name (cc_msgs_t id)
{
    if ((id <= CC_MSG_MIN) || (id >= CC_MSG_MAX)) {
        return get_debug_string(GSM_UNDEFINED);
    }

    return cc_msg_names[id];
}








const char *cc_cause_name (cc_causes_t id)
{
    if ((id <= CC_CAUSE_MIN) || (id >= CC_CAUSE_MAX)) {
        return get_debug_string(GSM_UNDEFINED);
    }

    return cc_cause_names[id - CC_CAUSE_OK];
}

