



#ifndef _CCAPI_DEVICE_H_
#define _CCAPI_DEVICE_H_

#include "ccapi_types.h"






cc_device_handle_t CCAPI_Device_getDeviceID();







cc_deviceinfo_ref_t CCAPI_Device_getDeviceInfo(cc_device_handle_t id);








void CCAPI_Device_configUpdate(cc_device_handle_t handle, file_path_t file_path);









 
void CCAPI_Device_retainDeviceInfo(cc_deviceinfo_ref_t ref);






void CCAPI_Device_releaseDeviceInfo(cc_deviceinfo_ref_t ref);







cc_call_handle_t CCAPI_Device_CreateCall(cc_device_handle_t handle);








void CCAPI_Device_enableVideo(cc_device_handle_t handle, cc_boolean enable);








void CCAPI_Device_enableCamera(cc_device_handle_t handle, cc_boolean enable);










void CCAPI_Device_setDigestNamePasswd (cc_device_handle_t handle,
                                       char *name, char *pw);





























void CCAPI_Device_IP_Update (cc_device_handle_t handle,
                              const char *signaling_ip,
                              const char *sig_interface,
                              int sig_int_type,
                              const char *media_ip,
                              const char *media_interface,
                              int media_int_type);









void CCAPI_Device_setVideoAutoTxPreference (cc_device_handle_t handle, cc_boolean txPref);

#endif 
