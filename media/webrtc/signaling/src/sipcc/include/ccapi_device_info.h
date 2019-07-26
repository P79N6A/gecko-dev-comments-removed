






































#ifndef _CCAPI_DEVICE_INFO_H_
#define _CCAPI_DEVICE_INFO_H_

#include "ccapi_types.h"





cc_deviceinfo_ref_t CCAPI_DeviceInfo_getDeviceHandle() ;







cc_string_t CCAPI_DeviceInfo_getDeviceName(cc_deviceinfo_ref_t handle) ;






cc_boolean CCAPI_DeviceInfo_isPhoneIdle(cc_deviceinfo_ref_t handle) ;






cc_service_state_t CCAPI_DeviceInfo_getServiceState(cc_deviceinfo_ref_t handle) ;






cc_service_cause_t CCAPI_DeviceInfo_getServiceCause(cc_deviceinfo_ref_t handle) ;






cc_cucm_mode_t CCAPI_DeviceInfo_getCUCMMode(cc_deviceinfo_ref_t handle) ;








void CCAPI_DeviceInfo_getCalls(cc_deviceinfo_ref_t handle, cc_call_handle_t handles[], cc_uint16_t *count) ;









void CCAPI_DeviceInfo_getCallsByState(cc_deviceinfo_ref_t handle, cc_call_state_t state, 
                cc_call_handle_t handles[], cc_uint16_t *count) ;








void CCAPI_DeviceInfo_getLines(cc_deviceinfo_ref_t handle, cc_lineid_t handles[], cc_uint16_t *count) ;








void CCAPI_DeviceInfo_getFeatures(cc_deviceinfo_ref_t handle, cc_featureinfo_ref_t handles[], cc_uint16_t *count) ;








void CCAPI_DeviceInfo_getCallServers(cc_deviceinfo_ref_t handle, cc_callserver_ref_t handles[], cc_uint16_t *count) ;







cc_string_t CCAPI_DeviceInfo_getCallServerName(cc_callserver_ref_t handle);






cc_cucm_mode_t CCAPI_DeviceInfo_getCallServerMode(cc_callserver_ref_t handle);






cc_ccm_status_t CCAPI_DeviceInfo_getCallServerStatus(cc_callserver_ref_t handle);






cc_string_t CCAPI_DeviceInfo_getNotifyPrompt(cc_deviceinfo_ref_t handle) ;






cc_uint32_t CCAPI_DeviceInfo_getNotifyPromptPriority(cc_deviceinfo_ref_t handle) ;






cc_uint32_t CCAPI_DeviceInfo_getNotifyPromptProgress(cc_deviceinfo_ref_t handle) ;







cc_boolean CCAPI_DeviceInfo_isMissedCallLoggingEnabled (cc_deviceinfo_ref_t handle);







cc_boolean CCAPI_DeviceInfo_isPlacedCallLoggingEnabled (cc_deviceinfo_ref_t handle);







cc_boolean CCAPI_DeviceInfo_isReceivedCallLoggingEnabled (cc_deviceinfo_ref_t handle);







long long CCAPI_DeviceInfo_getRegTime (cc_deviceinfo_ref_t handle);







cc_string_t CCAPI_DeviceInfo_getSignalingIPAddress(cc_deviceinfo_ref_t handle);






cc_boolean CCAPI_DeviceInfo_isCameraEnabled(cc_deviceinfo_ref_t handle);






cc_boolean CCAPI_DeviceInfo_isVideoCapEnabled(cc_deviceinfo_ref_t handle);






cc_boolean CCAPI_DeviceInfo_getMWILampState(cc_deviceinfo_ref_t handle);

#endif 
