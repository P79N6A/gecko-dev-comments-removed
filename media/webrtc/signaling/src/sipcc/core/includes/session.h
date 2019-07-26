



#ifndef _SESSION_H_
#define _SESSION_H_

#include "sessionConstants.h"
#include "sessionTypes.h"
#include "sessuri.h"












void sessionProviderCmd(sessionProvider_cmd_t *);











void sessionProviderState(provider_state_t *state);












session_id_t createSession(uri_t uri_info);











int closeSession(session_id_t sess_id);












void sessionCmd(sessionCmd_t *sCmd);













void invokeFeature(session_feature_t *feat);













void invokeProviderFeature(session_feature_t *feat);












void sessionUpdate(session_update_t *session);












void featureUpdate(feature_update_t *feature);












void sessionMgmt (session_mgmt_t *sess_mgmt);











void sessionSendInfo (session_send_info_t *send_info);












void sessionRcvdInfo (session_rcvd_info_t *rcvd_info);

#endif

