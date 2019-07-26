



#include "sdp_os_defs.h"
#include "sdp.h"
#include "sdp_private.h"











tinybool sdp_verify_conf_ptr (sdp_conf_options_t *conf_p) 
{
    if ((conf_p != NULL) && (conf_p->magic_num == SDP_MAGIC_NUM)) {
        return (TRUE);
    } else {
        sdp_log_errmsg(SDP_ERR_INVALID_CONF_PTR, NULL);
        return (FALSE);
    }
}













sdp_conf_options_t sdp_config_options;
void *sdp_init_config ()
{
    int i;
    sdp_conf_options_t *conf_p;

    conf_p = & sdp_config_options;

    
    conf_p->magic_num = SDP_MAGIC_NUM;

    
    conf_p->debug_flag[SDP_DEBUG_TRACE]    = FALSE;
    conf_p->debug_flag[SDP_DEBUG_WARNINGS] = FALSE;
    conf_p->debug_flag[SDP_DEBUG_ERRORS]   = FALSE;

    
    
    conf_p->version_reqd       = TRUE;
    conf_p->owner_reqd         = TRUE;
    conf_p->session_name_reqd  = TRUE;
    conf_p->timespec_reqd      = TRUE;

    
    for (i=0; i < SDP_MAX_MEDIA_TYPES; i++) {
        conf_p->media_supported[i] = FALSE;
    }

    
    for (i=0; i < SDP_MAX_NETWORK_TYPES; i++) {
        conf_p->nettype_supported[i] = FALSE;
    }

    
    for (i=0; i < SDP_MAX_ADDR_TYPES; i++) {
        conf_p->addrtype_supported[i] = FALSE;
    }

    
    for (i=0; i < SDP_MAX_TRANSPORT_TYPES; i++) {
        conf_p->transport_supported[i] = FALSE;
    }

    
    for (i=0; i < SDP_MAX_CHOOSE_PARAMS; i++) {
        conf_p->allow_choose[i] = FALSE;
    }

    
    conf_p->num_parses              = 0;
    conf_p->num_builds              = 0;
    conf_p->num_not_sdp_desc        = 0;
    conf_p->num_invalid_token_order = 0;
    conf_p->num_invalid_param       = 0;
    conf_p->num_no_resource         = 0;

    return (conf_p);
}













void sdp_appl_debug (void *config_p, sdp_debug_e debug_type, 
                     tinybool debug_flag)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    if (debug_type < SDP_MAX_DEBUG_TYPES)  {    
        conf_p->debug_flag[debug_type] = debug_flag;
    }
}













void sdp_require_version (void *config_p, tinybool version_required)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->version_reqd = version_required;
}

void sdp_require_owner (void *config_p, tinybool owner_required)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->owner_reqd = owner_required;
}

void sdp_require_session_name (void *config_p, tinybool sess_name_required)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->session_name_reqd = sess_name_required;
}

void sdp_require_timespec (void *config_p, tinybool timespec_required)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->timespec_reqd = timespec_required;
}











void sdp_media_supported (void *config_p, sdp_media_e media_type, 
			 tinybool media_supported)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->media_supported[media_type] = media_supported;
}













void sdp_nettype_supported (void *config_p, sdp_nettype_e nettype, 
			    tinybool nettype_supported)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->nettype_supported[nettype] = nettype_supported;
}













void sdp_addrtype_supported (void *config_p, sdp_addrtype_e addrtype, 
			     tinybool addrtype_supported)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->addrtype_supported[addrtype] = addrtype_supported;
}













void sdp_transport_supported (void *config_p, sdp_transport_e transport, 
			      tinybool transport_supported)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    conf_p->transport_supported[transport] = transport_supported;
}











void sdp_allow_choose (void *config_p, sdp_choose_param_e param, tinybool choose_allowed)
{
    sdp_conf_options_t *conf_p = (sdp_conf_options_t *)config_p;

    if (sdp_verify_conf_ptr(conf_p) == FALSE) {
        return;
    }

    if (param < SDP_MAX_CHOOSE_PARAMS) { 
        conf_p->allow_choose[param] = choose_allowed;
    }
}
