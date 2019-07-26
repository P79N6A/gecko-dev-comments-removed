






































#ifndef _CCSESSION_H_
#define _CCSESSION_H_

#include "session.h"
#include "sessuri.h"

#define SID_TYPE_SHIFT 28
#define SID_LINE_SHIFT 16

#define GET_SESS_TYPE(x) ( (x & 0xF0000000) >> SID_TYPE_SHIFT )
#define GET_LINEID(x) (line_t)( (x & 0xFFF0000) >> SID_LINE_SHIFT )
#define GET_CALLID(x) (callid_t)(x & 0xFFFF) 


















void ccSessionProviderCmd(sessionProvider_cmd_t *data); 














void ccSessionProviderState(unsigned int state, ccProvider_state_t *data); 














void ccSessionCmd (sessionCmd_t *sCmd);












session_id_t ccCreateSession(uri_t *param);












int ccCloseSession(session_id_t sess_id);














void ccInvokeFeature(session_feature_t *featData);












void ccInvokeProviderFeature(session_feature_t *featData);














void ccSessionUpdate(session_update_t *session);















void ccFeatureUpdate(feature_update_t *session);













session_id_t createSessionId(line_t line, callid_t call);

void platform_sync_cfg_vers (char *cfg_ver, char *dp_ver, char *softkey_ver);




#define PROPERTY_ID_MWI          1  // per line
#define PROPERTY_ID_TIME         2  // unsigned long
#define PROPERTY_ID_KPML         3
#define PROPERTY_ID_REGREASON    4
#define PROPERTY_ID_SPKR_HDST    5     1

void setIntProperty(unsigned int id, int  val);
int getIntProperty(unsigned int id);
void setStrProperty(unsigned int id, char * val);
char * getStrProperty(unsigned int id);



char *ccSetDP(const char *dp_file_name);

void setPropertyCacheBoolean(int cfg_id, int bool_value);	
void setPropertyCacheInteger(int cfg_id, int int_value);
void setPropertyCacheString(int cfg_id, const char *string_value);
void setPropertyCacheByte(int cfg_id, char byte_value);
void setPropertyCacheByteArray(int cfg_id, char *byte_value, int length);


void ccBLFSubscribe(int request_id, int duration, const char *watcher,
                    const char *presentity, int app_id, int feature_mask);
void ccBLFUnsubscribe(int request_id);
void ccBLFUnsubscribeAll();
void blf_notification(int request_id, int status, int app_id);








#endif

