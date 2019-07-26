






































#include "ccapi_snapshot.h"
#include "ccapi_line.h"
#include "sessionHash.h"
#include "CCProvider.h"
#include "phone_debug.h"






cc_int32_t CCAPI_lineInfo_getID(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getID";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
   
   
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->button);
       return info->button;
   }
   return -1;
}






cc_string_t CCAPI_lineInfo_getName(cc_lineinfo_ref_t line) {
   static const char *fname="CCAPI_lineInfo_getName";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->dn);
       return info->dn;
   }
   return NULL;
}







cc_string_t CCAPI_lineInfo_getLabel(cc_lineinfo_ref_t line) {
   static const char *fname="CCAPI_lineInfo_getLabel";
   cc_line_info_t  *info = (cc_line_info_t *) line;
   cc_string_t label = strlib_empty();

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       label = ccsnap_get_line_label(info->button);
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), label);
   }
   return label;
}






cc_string_t CCAPI_lineInfo_getNumber(cc_lineinfo_ref_t line) {
   static const char *fname="CCAPI_lineInfo_getNumber";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->name);
       return info->name;
   }
   return NULL;
}






cc_string_t CCAPI_lineInfo_getExternalNumber(cc_lineinfo_ref_t line) {
   static const char *fname="CCAPI_lineInfo_getExternalNumber";
   cc_line_info_t  *info = (cc_line_info_t *) line;
   char externalNumberMask[MAX_EXTERNAL_NUMBER_MASK_SIZE];
   memset(externalNumberMask, 0, sizeof(externalNumberMask));

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   config_get_string(CFGID_CCM_EXTERNAL_NUMBER_MASK, externalNumberMask, MAX_EXTERNAL_NUMBER_MASK_SIZE);
   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->name);
       if (strlen(externalNumberMask) > 0) {
           CCAPP_DEBUG(DEB_F_PREFIX"number with mask applied == %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->externalNumber);
           return info->externalNumber;
       } else {
           CCAPP_DEBUG(DEB_F_PREFIX"number without mask == %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->name);
           return info->name;
       }
   }
   return NULL;
}






cc_uint32_t CCAPI_lineInfo_getButton(cc_lineinfo_ref_t line){
   static const char *fname="CCAPI_lineInfo_getButton";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->button);
       return info->button;
   }
   return 0;

}






cc_line_feature_t CCAPI_lineInfo_getLineType(cc_lineinfo_ref_t line){
   static const char *fname="CCAPI_lineInfo_getLineType";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->line_type);
       return info->line_type;
   }
   return 0;
}








cc_boolean CCAPI_lineInfo_isCFWDActive(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_isCFWDActive";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->isCFWD);
       return info->isCFWD;
   }
   return FALSE;
}






cc_string_t CCAPI_lineInfo_getCFWDName(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getCFWDName";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %s\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->cfwd_dest);
       return info->cfwd_dest;
   }
   return NULL;
}






cc_uint32_t CCAPI_lineInfo_getMWIStatus(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getMWIStatus";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d, status %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->mwi, info->mwi.status);
       return info->mwi.status;
   }
   return 0;
}






cc_uint32_t CCAPI_lineInfo_getMWIType(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getMWIType";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d, type %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->mwi, info->mwi.type);
       return info->mwi.type;
   }
   return 0;
}






cc_uint32_t CCAPI_lineInfo_getMWINewMsgCount(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getMWINewMsgCount";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d, new count %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->mwi, info->mwi.new_count);
       return info->mwi.new_count;
   }
   return 0;
}





cc_uint32_t CCAPI_lineInfo_getMWIOldMsgCount(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getMWIOldMsgCount";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d, old_count %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->mwi, info->mwi.old_count);
       return info->mwi.old_count;
   }
   return 0;
}






cc_uint32_t CCAPI_lineInfo_getMWIPrioNewMsgCount(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getMWIPrioNewMsgCount";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d , pri_new count %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->mwi, info->mwi.pri_new_count);
       return info->mwi.pri_new_count;
   }
   return 0;
}





cc_uint32_t CCAPI_lineInfo_getMWIPrioOldMsgCount(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getMWIPrioOldMsgCount";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d, pri old_count %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->mwi, info->mwi.pri_old_count);
       return info->mwi.pri_old_count;
   }
   return 0;
}









void CCAPI_LineInfo_getCalls(cc_lineid_t line, cc_call_handle_t handles[], int *count)
{
    static const char *fname="CCAPI_Line_getCalls";
    hashItr_t itr;
    session_data_t *data;
    int i=0;

    CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

    hashItrInit(&itr);
    while ( (data = (session_data_t*)hashItrNext(&itr)) != NULL &&
              i<*count ) {
         if ( GET_LINE_ID(data->sess_id) == line ){
            handles[i++] = CREATE_CALL_HANDLE_FROM_SESSION_ID(data->sess_id);
         }
    }
    *count=i;
    CCAPP_DEBUG(DEB_F_PREFIX"Finished (no return) \n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
}








void CCAPI_LineInfo_getCallsByState(cc_lineid_t line, cc_call_state_t state,
                 cc_call_handle_t handles[], int *count)
{
    static const char *fname="CCAPI_Line_getCallsByState";
    hashItr_t itr;
    session_data_t *data;
    int i=0;

    CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

    hashItrInit(&itr);
    while ( (data = (session_data_t*)hashItrNext(&itr)) != NULL &&
              i<*count ) {
         if ( GET_LINE_ID(data->sess_id) == line && data->state ==state ){
            handles[i++] = CREATE_CALL_HANDLE_FROM_SESSION_ID(data->sess_id);
         }
    }
    *count=i;
    CCAPP_DEBUG(DEB_F_PREFIX"Finished (no return) \n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
}






cc_boolean CCAPI_lineInfo_getRegState(cc_lineinfo_ref_t line)
{
   static const char *fname="CCAPI_lineInfo_getRegState";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL ) {
       CCAPP_DEBUG(DEB_F_PREFIX"returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname), info->reg_state);
       return info->reg_state;
   }
   return 0;
}







cc_boolean  CCAPI_LineInfo_hasCapability (cc_lineinfo_ref_t line, cc_int32_t feat_id){
   static const char *fname="CCAPI_LineInfo_hasCapability";
   cc_line_info_t  *info = (cc_line_info_t *) line;

   CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

   if ( info != NULL){
     CCAPP_DEBUG(DEB_F_PREFIX"feature id:  %d , value returned %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),feat_id,  info->allowed_features[feat_id]);
      return info->allowed_features[feat_id];
    }

    return FALSE;
}








cc_return_t  CCAPI_LineInfo_getCapabilitySet (cc_lineinfo_ref_t line, cc_int32_t feat_set[]){
    static const char *fname="CCAPI_LineInfo_getCapabilitySet";
   cc_line_info_t  *info = (cc_line_info_t *) line;
    int feat_id;
	         
    CCAPP_DEBUG(DEB_F_PREFIX"Entering\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));

    if ( info != NULL){
       for (feat_id = 0; feat_id < CCAPI_CALL_CAP_MAX; feat_id++) {
         feat_set[feat_id] = info->allowed_features[feat_id];
         CCAPP_DEBUG(DEB_F_PREFIX"feature id:  %d , value %d\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname),feat_id,  feat_set[feat_id]);
       }
			            
       CCAPP_DEBUG(DEB_F_PREFIX"returned CC_SUCCESS\n", DEB_F_PREFIX_ARGS(SIP_CC_PROV, fname));
       return CC_SUCCESS;
    }

    return CC_FAILURE;
}


