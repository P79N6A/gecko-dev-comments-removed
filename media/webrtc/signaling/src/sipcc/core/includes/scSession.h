



#include "string_lib.h"
#include "sessionConstants.h"
#include "sessionTypes.h"


void scSessionProviderCmd(sessionProvider_cmd_t *data);


void scSessionProviderState(unsigned int state, scProvider_state_t *data);


session_id_t scCreateSession(session_create_param_t *param);
void scCloseSession(session_id_t sess_id);
void scInvokeFeature(session_feature_t *featData);


void scSessionUpdate(session_update_t *session);
void scFeatureUpdate(feature_update_t *data);


