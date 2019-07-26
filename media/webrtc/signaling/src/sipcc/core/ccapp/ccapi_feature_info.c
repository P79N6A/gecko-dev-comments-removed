



#include "ccapi_snapshot.h"
#include "sessionHash.h"
#include "CCProvider.h"
#include "phone_debug.h"






cc_int32_t CCAPI_featureInfo_getButton(cc_featureinfo_ref_t feature)
{
   static const char *fname="CCAPI_featureInfo_getButton";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->button);
       return info->button;
   }
   return -1;
}






cc_int32_t CCAPI_featureInfo_getFeatureID(cc_featureinfo_ref_t feature)
{
   static const char *fname="CCAPI_featureInfo_getFeatureID";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->feature_id);
       return info->feature_id;
   }
   return -1;
}






cc_string_t CCAPI_featureInfo_getDisplayName(cc_featureinfo_ref_t feature) {
   static const char *fname="CCAPI_featureInfo_getDisplayName";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->name);
       return ccsnap_get_line_label(info->button);
   }
   return NULL;
}






cc_string_t CCAPI_featureInfo_getSpeedDialNumber(cc_featureinfo_ref_t feature) {
   static const char *fname="CCAPI_featureInfo_getSpeedDialNumber";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->speedDialNumber);
       return info->speedDialNumber;
   }
   return NULL;
}






cc_string_t CCAPI_featureInfo_getContact(cc_featureinfo_ref_t feature) {
   static const char *fname="CCAPI_featureInfo_getContact";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->contact);
       return info->contact;
   }
   return NULL;
}






cc_string_t CCAPI_featureInfo_getRetrievalPrefix(cc_featureinfo_ref_t feature) {
   static const char *fname="CCAPI_featureInfo_getRetrievalPrefix";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->retrievalPrefix);
       return info->retrievalPrefix;
   }
   return NULL;
}






cc_blf_state_t CCAPI_featureInfo_getBLFState(cc_featureinfo_ref_t feature) {
   static const char *fname="CCAPI_featureInfo_getBLFState";
   cc_feature_info_t  *info = (cc_feature_info_t  *)feature;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->blf_state);
       return info->blf_state;
   }
   return CC_SIP_BLF_UNKNOWN;
}






cc_int32_t CCAPI_featureInfo_getFeatureOptionMask(cc_featureinfo_ref_t feature)
{
   static const char *fname="CCAPI_featureInfo_getFeatureOptionMask";
   cc_feature_info_t  *info;
   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   info = (cc_feature_info_t *) feature;
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->featureOptionMask);
       return info->featureOptionMask;
   }
   return -1;
}
