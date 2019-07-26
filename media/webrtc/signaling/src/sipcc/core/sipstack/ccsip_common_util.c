






































#include "cpr_in.h"
#include "ccsip_common_cb.h"
#include "sip_common_transport.h"
#include "prot_configmgr.h"
#include "ccsip_register.h"
#include "util_string.h"










void ccsip_common_util_set_dest_ipaddr_port (ccsip_common_cb_t *cb_p)
{
    char            addr[MAX_IPADDR_STR_LEN];

    if (cb_p->dest_sip_addr.type == CPR_IP_ADDR_INVALID) {
        sipTransportGetPrimServerAddress(cb_p->dn_line, addr);
        dns_error_code = sipTransportGetServerAddrPort(addr,
                                                       &cb_p->dest_sip_addr,
                                                       (uint16_t *)&cb_p->dest_sip_port,
                                                       &cb_p->SRVhandle,
                                                       FALSE);
        if (dns_error_code == 0) {
            util_ntohl(&(cb_p->dest_sip_addr), &(cb_p->dest_sip_addr));
        } else {
            sipTransportGetServerIPAddr(&(cb_p->dest_sip_addr), cb_p->dn_line);
        }

        cb_p->dest_sip_port = ((dns_error_code == 0) && (cb_p->dest_sip_port)) ?
                              ntohs((uint16_t)cb_p->dest_sip_port) :
                              (sipTransportGetPrimServerPort(cb_p->dn_line));
    }
}










void ccsip_common_util_set_src_ipaddr (ccsip_common_cb_t *cb_p)
{
    int             nat_enable = 0;

    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 0) {
        sip_config_get_net_device_ipaddr(&(cb_p->src_addr));
    } else {
        sip_config_get_nat_ipaddr(&(cb_p->src_addr));
    }
}






















void ccsip_common_util_set_retry_settings (ccsip_common_cb_t *cb_p, int *timeout_p)
{
    uint32_t max_retx = 0;
    const char *transport = NULL;

    *timeout_p = 0;
    cb_p->retx_flag = TRUE;
    config_get_value(CFGID_TIMER_T1, timeout_p, sizeof(*timeout_p));

    transport = sipTransportGetTransportType(cb_p->dn_line, TRUE, NULL);
    if (transport) {
        if (strcmp(transport, "UDP") == 0) {
            cb_p->retx_counter = 0;
        } else {
            config_get_value(CFGID_SIP_RETX, &max_retx, sizeof(max_retx));
            if (max_retx > MAX_NON_INVITE_RETRY_ATTEMPTS) {
                max_retx = MAX_NON_INVITE_RETRY_ATTEMPTS;
            }
            cb_p->retx_counter = max_retx;
            (*timeout_p) = (64 * (*timeout_p));
        }
    }
}















boolean ccsip_common_util_generate_auth (sipMessage_t *pSipMessage, ccsip_common_cb_t *cb_p,
                                         const char *rsp_method, int response_code, char *uri)
{
    static const char fname[] = "ccsip_common_util_generate_auth";
    const char     *authenticate = NULL;
    credentials_t   credentials;
    sip_authen_t   *sip_authen = NULL;
    char           *author_str = NULL;

    if (!(cb_p->authen.cred_type & CRED_LINE)) {
        cb_p->authen.cred_type |= CRED_LINE;
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX
                          "configured credentials for line %d not accepeted. Verify the config\n",
                          fname, cb_p->dn_line);
        return FALSE;
    }

    


    cred_get_line_credentials(cb_p->dn_line, &credentials,
                              sizeof(credentials.id),
                              sizeof(credentials.pw));
    


    authenticate = sippmh_get_header_val(pSipMessage, AUTH_HDR(response_code), NULL);
    if (authenticate == NULL) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%s header missing in the %d response\n",
                          fname, AUTH_HDR_STR(response_code), response_code);
        return FALSE;
    }
    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Authenticate header %s = %s\n", DEB_F_PREFIX_ARGS(SIP_AUTH, fname), AUTH_HDR_STR(response_code), authenticate);
    


    sip_authen = sippmh_parse_authenticate(authenticate);
    if (sip_authen == NULL) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"%s:%s header formatted incorrectly in the %d response\n",
                          fname, AUTH_HDR_STR(response_code), authenticate, response_code);
        return FALSE;
    }
    cb_p->authen.new_flag = FALSE;
    cb_p->authen.cnonce[0] = '\0';
    


    if (sipSPIGenerateAuthorizationResponse(sip_authen,
                                            uri,
                                            rsp_method,
                                            credentials.id,
                                            credentials.pw,
                                            &author_str,
                                            &(cb_p->authen.nc_count),
                                            NULL) == TRUE) {

        if (cb_p->authen.authorization != NULL) {
            cpr_free(cb_p->authen.authorization);
            cb_p->authen.authorization = NULL;
        }

        if (cb_p->authen.sip_authen != NULL) {
            sippmh_free_authen(cb_p->authen.sip_authen);
            cb_p->authen.sip_authen = NULL;
        }

        cb_p->authen.authorization = (char *)
        cpr_malloc(strlen(author_str) * sizeof(char) + 1);

        



        if (cb_p->authen.authorization != NULL) {
            memcpy(cb_p->authen.authorization, author_str,
                   strlen(author_str) * sizeof(char) + 1);
            cb_p->authen.status_code = response_code;
            cb_p->authen.sip_authen = sip_authen;
        }

        cpr_free(author_str);
    } else {
         CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Authorization header build unsuccessful\n", fname);
         sippmh_free_authen(sip_authen);
         return FALSE;
    }
    return TRUE;
}











void ccsip_util_get_from_entity (sipMessage_t *pSipMessage, char *entity)
{
    const char     *sip_from = NULL;
    sipLocation_t  *from_loc = NULL;

    sip_from = sippmh_get_cached_header_val(pSipMessage, FROM);
    if (sip_from != NULL) {
        from_loc = sippmh_parse_from_or_to((char *) sip_from, TRUE);
        if ((from_loc) && (from_loc->genUrl->schema == URL_TYPE_SIP) && (from_loc->genUrl->u.sipUrl->user)) {
            strncpy(entity, from_loc->genUrl->u.sipUrl->user, CC_MAX_DIALSTRING_LEN);       
        }
    }
    if (from_loc) {
        sippmh_free_location(from_loc);
    }
}











void ccsip_util_extract_user (char *url, char *user)
{
    genUrl_t *genUrl = NULL;

    genUrl = sippmh_parse_url(url, TRUE);
    if (genUrl != NULL) {
        strncpy(user, genUrl->u.sipUrl->user, CC_MAX_DIALSTRING_LEN);
        sippmh_genurl_free(genUrl);
    }
}
