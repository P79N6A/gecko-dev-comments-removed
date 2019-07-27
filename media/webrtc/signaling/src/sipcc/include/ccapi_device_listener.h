



#ifndef _CCAPI_DEVICE_LISTENER_H_
#define _CCAPI_DEVICE_LISTENER_H_

#include "cc_constants.h"












void CCAPI_DeviceListener_onDeviceEvent(ccapi_device_event_e type, cc_device_handle_t device_id, cc_deviceinfo_ref_t dev_info);












void CCAPI_DeviceListener_onFeatureEvent(ccapi_device_event_e type, cc_deviceinfo_ref_t device_info, cc_featureinfo_ref_t feature_info);

#endif 
