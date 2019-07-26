



#include "cc_config.h"
#include "CCProvider.h"
#include "phone_debug.h"
#include "cpr_types.h"
#include "dialplan.h"
#include "capability_set.h"
#include "configmgr.h"
#include "dialplanint.h"
#include "ccapp_task.h"
#include "cc_device_manager.h"
#include "config_api.h"

extern int config_parser_main( char *config, int complete_config);





int CC_Config_SetAvailableLines(cc_lineid_t lines) {
    ccappTaskPostMsg(CCAPP_UPDATELINES, &lines, sizeof(unsigned short), CCAPP_CCPROVIER);
    return CC_SUCCESS;
}









void CC_Config_setIntValue(int cfgid, int value) {
    config_set_value(cfgid, &value, sizeof(int));
    return;
}

void CC_Config_setBooleanValue(int cfgid, cc_boolean value) {
    int temp = (int) value;

    
    
    

    
    config_set_value(cfgid, &temp, sizeof(int));
    return;
}

void CC_Config_setStringValue(int cfgid, const char* value) {
    config_set_string(cfgid, (char *) value);
    return;
}

void CC_Config_setByteValue(int cfgid, unsigned char value) {
    int temp = (int) value;






    config_set_value(cfgid, &temp, sizeof(int));
    return;
}

void CC_Config_setArrayValue(int cfgid, char *byte_array, int length) {
    unsigned char *byte_ptr;
    int i;

    byte_ptr = cpr_malloc(length);
    if (byte_ptr == NULL) {
        TNP_DEBUG(DEB_F_PREFIX"setPropertyCacheByteArray():malloc failed.\n", DEB_F_PREFIX_ARGS(JNI, "nSetPropertyCacheByteArray"));
        return;
    }

    for (i = 0; i < length; i++) {
        byte_ptr[i] = (unsigned char) byte_array[i];
    }
    config_set_value(cfgid, byte_ptr, length);
    cpr_free(byte_ptr);

    return;
}







char* CC_Config_setDialPlan(const char *dial_plan_string, int length) {
    const char fname[] = "CC_Config_setDialPlan";
    int ret;

    



    if (dial_plan_string == NULL || length == 0 || length >= DIALPLAN_MAX_SIZE) {
        TNP_DEBUG(DEB_F_PREFIX"Setting NULL dialplan string (length [%d] is 0, or length is larger than maximum [%d])\n",
                DEB_F_PREFIX_ARGS(JNI, fname), length, DIALPLAN_MAX_SIZE);

        dp_init_template (NULL, 0);
        return (NULL);
     }

    ret = dp_init_template(dial_plan_string, length);
    TNP_DEBUG(DEB_F_PREFIX"Parsed dial_plan_string.  Version=[%s], Length=[%d]\n", DEB_F_PREFIX_ARGS(JNI, fname), g_dp_version_stamp, length);
    if (ret != 0)
    {
        return (NULL);
    }

    return (g_dp_version_stamp);
}








char* CC_Config_setFcp(const char *fcp_plan_string, int len) {
    const   char fname[] = "CC_Config_setFcp";
    int ret               = 0;

    



    TNP_DEBUG(DEB_F_PREFIX"FCP Parsing FCP doc\n", DEB_F_PREFIX_ARGS(JNI, fname));
    if (fcp_plan_string == NULL)
    {
        TNP_DEBUG(DEB_F_PREFIX"Null FCP xml document\n",
                DEB_F_PREFIX_ARGS(JNI, fname));

        fcp_init_template (NULL);
        return (NULL);
    }

    ret = fcp_init_template (fcp_plan_string);
    TNP_DEBUG(DEB_F_PREFIX"Parsed FCP xml.  Version=[%s]\n", DEB_F_PREFIX_ARGS(JNI, fname), g_fp_version_stamp);
    if (ret != 0)
    {
        return (NULL);
    }

    return (g_fp_version_stamp);
}
