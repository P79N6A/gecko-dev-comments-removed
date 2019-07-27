



#ifndef _CCAPIAPI_FEATURE_INFO_H_
#define _CCAPIAPI_FEATURE_INFO_H_

#include "ccapi_types.h"







cc_int32_t CCAPI_featureInfo_getButton(cc_featureinfo_ref_t feature);






cc_int32_t CCAPI_featureInfo_getFeatureID(cc_featureinfo_ref_t feature);





cc_string_t CCAPI_featureInfo_getDisplayName(cc_featureinfo_ref_t feature);






cc_string_t CCAPI_featureInfo_getSpeedDialNumber(cc_featureinfo_ref_t feature);






cc_string_t CCAPI_featureInfo_getContact(cc_featureinfo_ref_t feature);






cc_string_t CCAPI_featureInfo_getRetrievalPrefix(cc_featureinfo_ref_t feature);






cc_blf_state_t CCAPI_featureInfo_getBLFState(cc_featureinfo_ref_t feature);






cc_int32_t CCAPI_featureInfo_getFeatureOptionMask(cc_featureinfo_ref_t feature);


#endif 
