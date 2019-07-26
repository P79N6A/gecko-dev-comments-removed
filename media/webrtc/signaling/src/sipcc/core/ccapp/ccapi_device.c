






































#include "cpr_stdlib.h"
#include "string_lib.h"
#include "ccapi_snapshot.h"
#include "ccapi_device.h"
#include "CCProvider.h"
#include "cc_config.h"
#include "cc_call_feature.h"
#include "cc_device_feature.h"
#include "ccsip_messaging.h"
#include "ccapi_call_info.h"
#include "cc_device_manager.h"
#include "cc_service_listener.h"
#include "platform_api.h"
#include "util_string.h"
#include "ccapi_service.h"
#include "ccapi_device_info.h"
  
char g_new_signaling_ip[MAX_IPADDR_STR_LEN];

dock_undock_event_t  g_dock_undock_event = MEDIA_INTERFACE_UPDATE_NOT_REQUIRED;
extern accessory_cfg_info_t g_accessoryCfgInfo;

extern void escalateDeescalate();

int signaling_interface_type;





cc_device_handle_t CCAPI_Device_getDeviceID()
{
   return CC_DEVICE_ID;
}






cc_deviceinfo_ref_t CCAPI_Device_getDeviceInfo(cc_device_handle_t handle)
{
  cc_device_info_t *device_info = (cc_device_info_t*)cpr_malloc(sizeof(cc_device_info_t));

  if (device_info) {
     *device_info = g_deviceInfo;
      device_info->name  = strlib_copy(g_deviceInfo.name);
      if (device_info->name == NULL) {
          device_info->name  = strlib_empty();
      }
      device_info->not_prompt  = strlib_copy(g_deviceInfo.not_prompt);
      if (device_info->not_prompt == NULL) {
          device_info->not_prompt = strlib_empty();
      }
      device_info->ref_count = 1;
  }
  return device_info;
}







void CCAPI_Device_retainDeviceInfo(cc_deviceinfo_ref_t ref){
    cc_device_info_t *device_info = ref;
    if (device_info) {
        device_info->ref_count++;
    }
}







void CCAPI_Device_configUpdate(cc_device_handle_t handle, file_path_t file_path) {
    CC_Config_setStringValue(CFGID_CONFIG_FILE, file_path);
}







void CCAPI_Device_releaseDeviceInfo(cc_deviceinfo_ref_t ref){
    cc_device_info_t *device_info = ref;

    if (device_info) {
       device_info->ref_count--;
       if ( device_info->ref_count == 0 ) {
            strlib_free(device_info->name);
            strlib_free(device_info->not_prompt);
            cpr_free(device_info);
        }
    }
}







cc_call_handle_t CCAPI_Device_CreateCall(cc_device_handle_t handle) 
{
  return CC_createCall(0);
}








void CCAPI_Device_enableVideo(cc_device_handle_t handle, cc_boolean enable)
{
   CC_DeviceFeature_enableVideo(enable);
   g_accessoryCfgInfo.video = ACCSRY_CFGD_APK;
}








void CCAPI_Device_enableCamera(cc_device_handle_t handle, cc_boolean enable)
{
   CC_DeviceFeature_enableCamera(enable);
   g_accessoryCfgInfo.camera = ACCSRY_CFGD_APK;
}









void CCAPI_Device_setDigestNamePasswd (cc_device_handle_t handle,
                                       char *name, char *pw)
{
    int line;

    for(line = 0; line < MAX_CONFIG_LINES; line++) {
        CC_Config_setStringValue(CFGID_LINE_AUTHNAME + line, name);
        CC_Config_setStringValue(CFGID_LINE_PASSWORD + line, pw);
    }
}




























void CCAPI_Device_IP_Update (cc_device_handle_t handle,
                              const char *signaling_ip,
                              const char *signaling_interface,
                              int signaling_int_type,
                              const char *media_ip,
                              const char *media_interface,
                              int media_int_type)
{
    static const char fname[] = "CCAPI_Device_IP_Update";
    char curr_signaling_ip[MAX_IPADDR_STR_LEN];
    char curr_media_ip[MAX_IPADDR_STR_LEN];
    cpr_ip_addr_t      sig_ip;

    signaling_interface_type = signaling_int_type;

    
    init_empty_str(curr_signaling_ip);
    init_empty_str(curr_media_ip);
    init_empty_str(g_new_signaling_ip);

    config_get_value(CFGID_MY_IP_ADDR, &sig_ip, sizeof(cpr_ip_addr_t));
    sig_ip.type = CPR_IP_ADDR_IPV4;
    util_ntohl(&sig_ip, &sig_ip);
    ipaddr2dotted(curr_signaling_ip, &sig_ip);

    config_get_string(CFGID_MEDIA_IP_ADDR, curr_media_ip,
                        MAX_IPADDR_STR_LEN);

    DEF_DEBUG(DEB_F_PREFIX"New sig_ip=%s media_ip=%s  Current: sig_ip: %s,"\
            "media_ip: %s \n", 
            DEB_F_PREFIX_ARGS(CC_API, fname),
            signaling_ip,
            media_ip,
            curr_signaling_ip, 
            curr_media_ip);

    



    if ((is_empty_str((char *)signaling_ip) ||
        (strncmp(signaling_ip, "0.0.0.0", MAX_IPADDR_STR_LEN) == 0))
        && (is_empty_str((char *)media_ip) ||
        (strncmp(media_ip, "0.0.0.0", MAX_IPADDR_STR_LEN) == 0))) {
        CC_Config_setStringValue(CFGID_MY_IP_ADDR, "0.0.0.0");
        CC_Config_setStringValue(CFGID_MEDIA_IP_ADDR, EMPTY_STR);
        DEF_DEBUG(DEB_F_PREFIX"Media and Signaling IP Not provided."\
                  "Shutdown sip stack", DEB_F_PREFIX_ARGS(CC_API, fname));
        if ((strncmp(curr_signaling_ip, signaling_ip,
              MAX_IPADDR_STR_LEN) != 0)) {
            registration_processEvent(EV_CC_IP_INVALID);
            return;
        }
    }
  
    



    if ((signaling_ip != NULL)  &&
        (strncmp(curr_signaling_ip, signaling_ip, MAX_IPADDR_STR_LEN) != 0)) {
        CC_Config_setStringValue(CFGID_MY_IP_ADDR, signaling_ip);
        DEF_DEBUG(DEB_F_PREFIX"Signaling IP changed. Re-register, if needed.",
                  DEB_F_PREFIX_ARGS(CC_API, fname));
        registration_processEvent(EV_CC_IP_VALID);

    }

    




    if ((media_ip != NULL)  &&
        (strncmp(curr_media_ip, media_ip, MAX_IPADDR_STR_LEN) != 0)) {
        CC_Config_setStringValue(CFGID_MEDIA_IP_ADDR, media_ip);
        if (g_dock_undock_event != MEDIA_INTERFACE_UPDATE_IN_PROCESS) {
            g_dock_undock_event = MEDIA_INTERFACE_UPDATE_STARTED;
            DEF_DEBUG(DEB_F_PREFIX" MEDIA_INTERFACE_UPDATE received. escalateDeescalate.",
                      DEB_F_PREFIX_ARGS(CC_API, fname));
            escalateDeescalate();
        }else {
            DEF_DEBUG(DEB_F_PREFIX"MEDIA_INTERFACE_UPDATE received but escalateDeescalate already in progress:%d",
                  DEB_F_PREFIX_ARGS(CC_API, fname), g_dock_undock_event);
        }
    }
}








void CCAPI_Device_setVideoAutoTxPreference (cc_device_handle_t handle, cc_boolean txPref)
{
        CCAPP_DEBUG("CCAPI_Device_setVideoAutoTxPreference: updated to %d\n", txPref);
	cc_media_setVideoAutoTxPref(txPref);
}


