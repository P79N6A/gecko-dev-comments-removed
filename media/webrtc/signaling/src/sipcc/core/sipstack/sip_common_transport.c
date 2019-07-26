



#include "cpr_in.h"
#include "cpr_stdlib.h"
#include "cpr_string.h"
#include "cpr_errno.h"
#include "cpr_socket.h"
#include "cpr_locks.h"
#include "phone_debug.h"
#include "dialplan.h"
#include "config.h"
#include "configmgr.h"
#include "ccsip_platform_udp.h"
#include "ccsip_platform_timers.h"
#include "ccsip_register.h"
#include "sip_platform_task.h"
#include "ccsip_task.h"
#include "ccsip_messaging.h"
#include "ccsip_reldev.h"
#include "sip_common_transport.h"
#include "sip_csps_transport.h"
#include "sip_ccm_transport.h"
#include "sip_common_regmgr.h"
#include "util_string.h"
#include "dns_utils.h"
#include "debug.h"
#include "phntask.h"
#include "ccsip_subsmanager.h"
#include "ccsip_publish.h"
#include "ccsip_platform_tcp.h"
#include "ccsip_platform_tls.h"
#include "platform_api.h"
#include "sessionTypes.h"

uint16_t ccm_config_id_addr_str[MAX_CCM] = {
    CFGID_CCM1_ADDRESS,
    CFGID_CCM2_ADDRESS,
    CFGID_CCM3_ADDRESS
};

#ifdef IPV6_STACK_ENABLED

uint16_t ccm_config_id_ipv6_addr_str[MAX_CCM] = {
    CFGID_CCM1_IPV6_ADDRESS,
    CFGID_CCM2_IPV6_ADDRESS,
    CFGID_CCM3_IPV6_ADDRESS
};

#endif

uint16_t ccm_config_id_port[MAX_CCM] = {
    CFGID_CCM1_SIP_PORT,
    CFGID_CCM2_SIP_PORT,
    CFGID_CCM3_SIP_PORT
};

uint16_t ccm_config_id_sec_level[MAX_CCM] = {
    CFGID_CCM1_SEC_LEVEL,
    CFGID_CCM2_SEC_LEVEL,
    CFGID_CCM3_SEC_LEVEL
};

uint16_t ccm_config_id_is_valid[MAX_CCM] = {
    CFGID_CCM1_IS_VALID,
    CFGID_CCM2_IS_VALID,
    CFGID_CCM3_IS_VALID
};

ti_config_table_t CCM_Device_Specific_Config_Table[MAX_CCM];
ti_config_table_t CCM_Dummy_Entry;
ti_config_table_t CSPS_Config_Table[MAX_REG_LINES];
ti_config_table_t *CCM_Config_Table[MAX_REG_LINES + 1][MAX_CCM];
int phone_local_tcp_port[UNUSED_PARAM];
ti_csps_t CSPS_Device_Specific_Config_Table;

sip_tcp_conn_t sip_tcp_conn_tab[MAX_CONNECTIONS];

char sent_string[] = "Sent:";
char rcvd_string[] = "Rcvd:";
char cseq_string[] = " Cseq:";
char callid_string[] = " CallId:";

static cpr_socket_t listen_socket = INVALID_SOCKET;
extern cc_config_table_t CC_Config_Table[];
extern sipCallHistory_t gCallHistory[];
extern ccm_act_stdby_table_t CCM_Active_Standby_Table;



extern void platAddCallControlClassifiers(
            unsigned long myIPAddr, unsigned short myPort,
            unsigned long cucm1IPAddr, unsigned short cucm1Port,
            unsigned long cucm2IPAddr, unsigned short cucm2Port,
            unsigned long cucm3IPAddr, unsigned short cucm3Port,
            unsigned char  protocol);
extern void platform_get_ipv4_address(cpr_ip_addr_t *ip_addr);

#define SIP_IPPROTO_UDP 17
#define SIP_IPPROTO_TCP 6
            










void ccsip_add_wlan_classifiers ()
{
    cpr_ip_addr_t    my_addr;
    uint32_t    local_port = 0;
    uint32_t  transport_prot = 0;

    platform_get_ipv4_address (&my_addr);
    
    config_get_value(CFGID_VOIP_CONTROL_PORT, &local_port,
                         sizeof(local_port));
    config_get_value(CFGID_TRANSPORT_LAYER_PROT, &transport_prot,
                         sizeof(transport_prot));
    
    platAddCallControlClassifiers(my_addr.u.ip4, local_port ,
            ntohl(CCM_Device_Specific_Config_Table[PRIMARY_CCM].ti_common.addr.u.ip4),
            CCM_Device_Specific_Config_Table[PRIMARY_CCM].ti_common.port,
            ntohl(CCM_Device_Specific_Config_Table[SECONDARY_CCM].ti_common.addr.u.ip4),
            CCM_Device_Specific_Config_Table[SECONDARY_CCM].ti_common.port,
            ntohl(CCM_Device_Specific_Config_Table[TERTIARY_CCM].ti_common.addr.u.ip4),
            CCM_Device_Specific_Config_Table[TERTIARY_CCM].ti_common.port,
            (transport_prot==CONN_UDP)? SIP_IPPROTO_UDP:SIP_IPPROTO_TCP);
}

void ccsip_remove_wlan_classifiers ()
{
    platRemoveCallControlClassifiers();

}














static cpr_socket_t
sipTransportGetServerHandle (line_t dn, line_t ndx)
{

    ti_config_table_t *ccm_table_ptr = NULL;
    ti_common_t ti_common;
    static const char *fname = "sipTransportGetServerHandle";

    



    if (((int)dn < 1) || ((int)dn > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, dn);
        return (INVALID_SOCKET);
    }

    if (CC_Config_Table[dn - 1].cc_type == CC_CCM) {
        


        if (ndx == REG_BACKUP_CCB) {
            



            ccm_table_ptr = CCM_Active_Standby_Table.standby_ccm_entry;
        } else if (ndx > REG_BACKUP_CCB) {
            ccsipCCB_t *ccb = NULL;

            ccb = sip_sm_get_ccb_by_index(ndx);
            if (ccb != NULL) {
                ccm_table_ptr = (ti_config_table_t *) ccb->cc_cfg_table_entry;
            } else {
                return (INVALID_SOCKET);
            }
        } else {
            ccm_table_ptr = CCM_Active_Standby_Table.active_ccm_entry;
        }
        if (ccm_table_ptr) {
            ti_common = ccm_table_ptr->ti_common;
            return (ti_common.handle);
        }
    } else {
        


        return (sipTransportCSPSGetProxyHandleByDN(dn));
    }
    return (INVALID_SOCKET);
}













static cpr_socket_t
sipTransportGetServerHandleWithAddr (cpr_ip_addr_t *remote_ip_addr)
{

    ti_config_table_t *ccm_table_ptr = NULL;

    ccm_table_ptr = CCM_Active_Standby_Table.active_ccm_entry;

    if (ccm_table_ptr && util_compare_ip(remote_ip_addr, &(ccm_table_ptr->ti_common.addr))) {
        return (ccm_table_ptr->ti_common.handle);
    }
    ccm_table_ptr = CCM_Active_Standby_Table.standby_ccm_entry;
    if (ccm_table_ptr && util_compare_ip(remote_ip_addr, &(ccm_table_ptr->ti_common.addr))) {
        return (ccm_table_ptr->ti_common.handle);
    }
    return (INVALID_SOCKET);
}














uint16_t
sipTransportGetServerAddress (cpr_ip_addr_t *pip_addr, line_t dn, line_t ndx)
{
    static const char *fname = "sipTransportGetServerAddress";
    *pip_addr = ip_addr_invalid;

    



    if (((int)dn < 1) || ((int)dn > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, dn);
        return (0);
    }

    if (CC_Config_Table[dn - 1].cc_type == CC_CCM) {
        


        if (ndx == REG_BACKUP_CCB) {
            



            if (CCM_Active_Standby_Table.standby_ccm_entry) {
                *pip_addr = CCM_Active_Standby_Table.standby_ccm_entry->
                        ti_common.addr;
            }
            return (0);
        } else if (ndx > REG_BACKUP_CCB) {
            ccsipCCB_t *ccb = NULL;

            ccb = sip_sm_get_ccb_by_index(ndx);
            if (ccb != NULL) {
                ti_config_table_t *ccm_table_ptr = NULL;
                ti_common_t ti_common;

                ccm_table_ptr = (ti_config_table_t *) ccb->cc_cfg_table_entry;
                if (ccm_table_ptr) {
                    ti_common = ccm_table_ptr->ti_common;
                    *pip_addr = ti_common.addr;
                }
            }
            return (0);
        } else {
            if (CCM_Active_Standby_Table.active_ccm_entry) {
                *pip_addr = CCM_Active_Standby_Table.active_ccm_entry->
                        ti_common.addr;
            }
            return (0);
        }
    } else {
        


        return (sipTransportCSPSGetProxyAddressByDN(pip_addr, dn));
    }
}














short
sipTransportGetServerPort (line_t dn, line_t ndx)
{
    static const char *fname = "sipTransportGetServerPort";
    



    if (((int)dn < 1) || ((int)dn > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, dn);
        return (0);
    }

    if (CC_Config_Table[dn - 1].cc_type == CC_CCM) {
        


        if (ndx == REG_BACKUP_CCB) {
            



            if (CCM_Active_Standby_Table.standby_ccm_entry) {
                return ((short) CCM_Active_Standby_Table.standby_ccm_entry->
                        ti_common.port);
            }
        } else if (ndx > REG_BACKUP_CCB) {
            ccsipCCB_t *ccb = NULL;

            ccb = sip_sm_get_ccb_by_index(ndx);
            if (ccb != NULL) {
                ti_config_table_t *ccm_table_ptr = NULL;
                ti_common_t ti_common;

                ccm_table_ptr = (ti_config_table_t *) ccb->cc_cfg_table_entry;
                if (ccm_table_ptr) {
                    ti_common = ccm_table_ptr->ti_common;
                    return (ti_common.port);
                }
            }
            return (0);
        } else {
            if (CCM_Active_Standby_Table.active_ccm_entry) {
                return ((short) CCM_Active_Standby_Table.active_ccm_entry->
                        ti_common.port);
            }
            return (0);
        }
    }

    


    return ((short) sipTransportCSPSGetProxyPortByDN(dn));
}

conn_create_status_t
sip_transport_setup_cc_conn (line_t dn, CCM_ID ccm_id)
{
    static const char *fname = "sip_transport_setup_cc_conn";
    int             dnsErrorCode;
    cpr_ip_addr_t   server_ipaddr;
    uint16_t        server_port = 0, listener_port = 0;
    cpr_socket_t    server_conn_handle = INVALID_SOCKET;
    conn_create_status_t status = CONN_INVALID;
    uint32_t        type;
    ti_common_t    *ti_common;
    int            ip_mode = CPR_IP_MODE_IPV4;
	uint32_t		s_port;

    



    if (((int)dn < 1) || ((int)dn > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, dn);
        return (status);
    }

    if (ccm_id >= MAX_CCM) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"ccm id <%d> out of bounds.\n",
                          fname, ccm_id);
        return (status);
    }
    CPR_IP_ADDR_INIT(server_ipaddr);

#ifdef IPV6_STACK_ENABLED

    config_get_value(CFGID_IP_ADDR_MODE, 
                     &ip_mode, sizeof(ip_mode));
#endif

    if (CC_Config_Table[dn - 1].cc_type == CC_CCM) {
        


        ti_ccm_t *ti_ccm;

        ti_ccm = &CCM_Config_Table[dn - 1][ccm_id]->ti_specific.ti_ccm;
        if (!ti_ccm->is_valid) {
            



            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Admin has not configured a valid cucm for cucm index=%s=%d.\n",
                              fname, CCM_ID_PRINT(ccm_id), ccm_id);
            return (status);
        }
        dnsErrorCode = dnsGetHostByName(CCM_Config_Table[dn - 1][ccm_id]->
                                         ti_common.addr_str, &server_ipaddr,
                                         100, 1);
        if (dnsErrorCode != DNS_OK) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              "sip_transport_setup_cc_conn",
                                "dnsGetHostByName() returned error:%s", 
                                CCM_Config_Table[dn - 1][ccm_id]->ti_common.addr_str);
            return status;
        }

        util_ntohl(&server_ipaddr, &server_ipaddr);

        
        config_get_value(CFGID_VOIP_CONTROL_PORT, &s_port, sizeof(s_port));
        server_port =  (uint16_t) s_port;

        if (CCM_Config_Table[dn - 1][ccm_id]->ti_common.conn_type == CONN_UDP) {
            type = SOCK_DGRAM;
            listener_port = CCM_Config_Table[dn - 1][ccm_id]->ti_common.listen_port;
        } else {
            type = SOCK_STREAM;
        }
    } else {
        


        sipTransportGetServerIPAddr(&server_ipaddr, dn);
        server_port   = (uint16_t) sipTransportGetPrimServerPort(dn);
        if (CSPS_Config_Table[dn - 1].ti_common.conn_type == CONN_UDP) {
            type = SOCK_DGRAM;
            listener_port = CSPS_Config_Table[dn - 1].ti_common.listen_port;
        } else {
            type = SOCK_STREAM;
        }
    }
    if (util_check_if_ip_valid(&server_ipaddr) && server_port != 0) {
        char server_ipaddr_str[MAX_IPADDR_STR_LEN];
        int  ret_status = SIP_ERROR;

        ipaddr2dotted(server_ipaddr_str, &server_ipaddr);
        if (type == SOCK_DGRAM) {
            ret_status = sip_platform_udp_channel_create(ip_mode, &server_conn_handle,
                                                         &server_ipaddr,
                                                         server_port, 0);
            if (ret_status == SIP_OK) {
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"DN <%d>: CC UDP socket opened: "
                                 "<%s>:<%d>, handle=<%d>\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), dn,
                                 server_ipaddr_str, server_port,
                                 server_conn_handle);
                status = CONN_SUCCESS;
            } else {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"DN <%d>:"
                                  "udp channel error"
                                  "server addr=%s, server port=%d) failed.\n",
                                  fname, dn, server_ipaddr_str, server_port);
                server_conn_handle = INVALID_SOCKET;
                status = CONN_FAILURE;
            }
        }
        else {
            sipSPIMessage_t sip_msg;

            if (CC_Config_Table[dn - 1].cc_type != CC_CCM) {
                
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"TLS and TCP not supported in non-ccm"
                                  " mode\n", fname);
                return (CONN_INVALID);
            }

            
            if (((CCM_Config_Table[dn - 1][ccm_id]->ti_specific.ti_ccm.sec_level
                    == AUTHENTICATED) ||
                 (CCM_Config_Table[dn - 1][ccm_id]->ti_specific.ti_ccm.sec_level
                    == ENCRYPTED)) &&
                (CCM_Config_Table[dn - 1][ccm_id]->ti_common.conn_type == CONN_TLS)) {
                uint32_t port = 0;

                CCSIP_DEBUG_TASK(DEB_F_PREFIX"server_ipaddr %d \n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), server_ipaddr);
                sip_msg.createConnMsg.addr = server_ipaddr;
                config_get_value(ccm_config_id_port[ccm_id], &port,
                                 sizeof(port));
                sip_msg.createConnMsg.port = (uint16_t) port;
                sip_msg.context = NULL;
                server_conn_handle = sip_tls_create_connection(&sip_msg, TRUE,
                        CCM_Config_Table[dn - 1][ccm_id]->ti_specific.ti_ccm.sec_level);
                if (server_conn_handle != INVALID_SOCKET) {
                    CCM_Config_Table[dn - 1][ccm_id]->ti_common.port =
                        (uint16_t) port;
                }
            } else {
                sip_msg.createConnMsg.addr = server_ipaddr;
                sip_msg.createConnMsg.port = server_port;
                sip_msg.context = NULL;
                server_conn_handle = sip_tcp_create_connection(&sip_msg);
            }
            if (server_conn_handle != INVALID_SOCKET) {
                listener_port = sip_msg.createConnMsg.local_listener_port;
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"DN <%d>: CC TCP socket opened: "
                                 "to <%s>:<%d>, local_port: %d handle=<%d>\n",
                                 DEB_F_PREFIX_ARGS(SIP_TRANS, fname), dn, server_ipaddr_str,
                                 server_port, listener_port,
                                 server_conn_handle);
                status = CONN_SUCCESS;
                phone_local_tcp_port[CCM_Config_Table[dn-1][ccm_id]->ti_specific.ti_ccm.ccm_id] = 
                    sip_msg.createConnMsg.local_listener_port;
            } else {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"DN <%d>:"
                                  "tcp channel create error "
                                  "server addr=%s, server port=%d) failed.\n",
                                  fname, dn, server_ipaddr_str, server_port);
                status = CONN_FAILURE;
            }
        }
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"DN <%d>: CC address/port not configured.\n",
                         fname, dn);
        status = CONN_INVALID;
    }

    if ((status == CONN_SUCCESS) || (status == CONN_FAILURE)) {
        if (CC_Config_Table[dn - 1].cc_type == CC_CCM) {
            


            



            ti_common = &CCM_Config_Table[dn - 1][ccm_id]->ti_common;
        } else {
            


            ti_common = &CSPS_Config_Table[dn - 1].ti_common;
        }
        ti_common->addr = server_ipaddr;
        ti_common->port = server_port;
        ti_common->handle = server_conn_handle;
        ti_common->listen_port = listener_port;
    }

    return (status);
}


int
sip_transport_destroy_cc_conn (line_t dn, CCM_ID ccm_id)
{
    static const char *fname = "sip_transport_destroy_cc_conn";
    int          disconnect_status = 0;
    cpr_socket_t cc_handle;
    CONN_TYPE    conn_type;
    uint16_t     max_cc_count, cc_index;
    ti_common_t *ti_common;
    CC_ID        cc_type;

    



    if (((int)dn < 1) || ((int)dn > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, dn);
        return (disconnect_status);
    }

    if (ccm_id >= MAX_CCM) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"ccm id <%d> out of bounds.\n",
                          fname, ccm_id);
        return (disconnect_status);
    }

    cc_type = CC_Config_Table[dn - 1].cc_type;
    if (cc_type == CC_CCM) {
        


        ti_common = &CCM_Config_Table[dn - 1][ccm_id]->ti_common;
        max_cc_count = MAX_CCM;
    } else {
        


        ti_common = &CSPS_Config_Table[dn - 1].ti_common;
        max_cc_count = MAX_CSPS;
    }
    cc_index = 0;
    do {
        cc_handle = ti_common->handle;
        conn_type = ti_common->conn_type;
        if (cc_handle != INVALID_SOCKET) {
            
            if (sip_platform_udp_channel_destroy(cc_handle) < 0) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"DN <%d>:"
                                  "handle=%d) \n", fname, dn, cc_handle);
                disconnect_status = -1;
            } else {
                CCSIP_DEBUG_TASK(DEB_F_PREFIX"DN <%d>: CC socket closed: "
                                 "handle=<%d>\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), dn, cc_handle);
                disconnect_status = 0;
            }
            if (conn_type != CONN_UDP) {
                int connid;

                connid = sip_tcp_fd_to_connid(ti_common->handle);
                sipTcpFreeSendQueue(connid);
                sip_tcp_purge_entry(connid);
            }
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"DN <%d>: CC socket already closed.\n",
                             DEB_F_PREFIX_ARGS(SIP_TRANS, fname), dn);
            disconnect_status = 0;
        }
        cc_index++;
        ti_common = &CCM_Config_Table[dn - 1][cc_index]->ti_common;
        


    } while (cc_index < max_cc_count);
    if (listen_socket != INVALID_SOCKET) {
        if (sip_platform_udp_channel_destroy(listen_socket) < 0) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"DN <%d>:"
                              "(handle=%d)\n", fname, dn, listen_socket);
            disconnect_status = -1;
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"DN <%d>: CC socket closed: handle=<%d>\n",
                             DEB_F_PREFIX_ARGS(SIP_TRANS, fname), dn, listen_socket);
            disconnect_status = 0;
        }
        sip_platform_task_reset_listen_socket(listen_socket);
        listen_socket = INVALID_SOCKET;
    }
    if (CC_Config_Table[dn - 1].cc_type == CC_CCM) {
        


        CCM_Config_Table[dn - 1][ccm_id]->ti_common.handle = INVALID_SOCKET;
    } else {
        ti_common = &CSPS_Config_Table[dn - 1].ti_common;
        ti_common->addr = ip_addr_invalid;
        ti_common->port = 0;
        ti_common->handle = INVALID_SOCKET;
    }
    return (disconnect_status);
}






















int
sipTransportCreateSendMessage (ccsipCCB_t *ccb,
                               sipMessage_t *pSIPMessage,
                               sipMethod_t message_type,
                               cpr_ip_addr_t *cc_remote_ipaddr,
                               uint16_t cc_remote_port,
                               boolean isRegister,
                               boolean reTx,
                               int timeout,
                               void *cbp,
                               int reldev_stored_msg)
{
    const char *fname = "sipTransportCreateSendMessage";
    static char aOutBuf[SIP_UDP_MESSAGE_SIZE + 1];
    uint32_t    nbytes = SIP_UDP_MESSAGE_SIZE;
    hStatus_t   sippmh_write_status = STATUS_FAILURE;

    


    if (!pSIPMessage) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args: pSIPMessage is null\n", fname);
        return (-1);
    }

    




    nbytes = sipRelDevGetStoredCoupledMessage(reldev_stored_msg, &aOutBuf[0],
                                              nbytes);
    if (nbytes == 0) {
        nbytes = SIP_UDP_MESSAGE_SIZE;
        sippmh_write_status = sippmh_write(pSIPMessage, aOutBuf, &nbytes);
    } else {
        sippmh_write_status = STATUS_SUCCESS;
    }
    ccsip_dump_send_msg_info(aOutBuf, pSIPMessage, cc_remote_ipaddr,
                            cc_remote_port);

    free_sip_message(pSIPMessage);
    if (sippmh_write_status == STATUS_FAILURE) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED),
                          ccb ? ccb->index : 0, ccb ? ccb->dn_line : 0, fname,
                          "sippmh_write()");
        return (-1);
    }
    if ((aOutBuf[0] == '\0') || (nbytes == 0)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sippmh_write() returned empty buffer "
                          "string\n", fname);
        return (-1);
    }
    aOutBuf[nbytes] = '\0'; 

    if (sipTransportSendMessage(ccb, aOutBuf, nbytes, message_type,
                                cc_remote_ipaddr, cc_remote_port, isRegister,
                                reTx, timeout, cbp) < 0) {
        if (ccb) {
            CCSIP_DEBUG_ERROR("SIPCC-ENTRY: LINE %d/%d: %-35s: message not "
                "sent of type %s=%d. sipTransportSendMessage() failed.\n",
                 ccb->index, ccb->dn_line, fname, 
                 message_type == sipMethodRegister ? "sipMethodRegister" : "", sipMethodRegister);
        } else {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sipTransportSendMessage()");
        }
        return (-1);
    }

    return (0);
}



















int
sipTransportSendMessage (ccsipCCB_t *ccb,
                         char *pOutMessageBuf,
                         uint32_t nbytes,
                         sipMethod_t message_type,
                         cpr_ip_addr_t *cc_remote_ipaddr,
                         uint16_t cc_remote_port,
                         boolean isRegister,
                         boolean reTx,
                         int timeout,
                         void *cbp)
{
    const char *fname = "sipTransportSendMessage";
    char         cc_config_ipaddr_str[MAX_IPADDR_STR_LEN];
    char         cc_remote_ipaddr_str[MAX_IPADDR_STR_LEN];
    char         obp_address[MAX_IPADDR_STR_LEN];
    cpr_socket_t send_to_proxy_handle = INVALID_SOCKET;
    int          nat_enable, dnsErrorCode;
    const char  *conn_type;
    uint32_t     local_udp_port = 0;
    int          tcp_error = SIP_TCP_SEND_OK;
    int          ip_mode = CPR_IP_MODE_IPV4;

    


    if ((!pOutMessageBuf) || (pOutMessageBuf[0] == '\0')) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args: pOutMessageBuf is empty\n", fname);
        return (-1);
    }

#ifdef IPV6_STACK_ENABLED
    config_get_value(CFGID_IP_ADDR_MODE, 
                     &ip_mode, sizeof(ip_mode));
#endif
    conn_type = sipTransportGetTransportType(1, TRUE, ccb);
    if (ccb) {
        




        




        cpr_ip_addr_t cc_config_ipaddr;
        uint16_t cc_config_port;

        sipTransportGetServerAddress(&cc_config_ipaddr, ccb->dn_line, ccb->index);
        cc_config_port   = sipTransportGetServerPort(ccb->dn_line, ccb->index);

        


        if (SipDebugMessage) {
            ipaddr2dotted(cc_config_ipaddr_str, &cc_config_ipaddr);
            ipaddr2dotted(cc_remote_ipaddr_str, cc_remote_ipaddr);
        }

        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"ccb <%d>: config <%s>:<%d> - remote <%s>:<%d>\n",
                            DEB_F_PREFIX_ARGS(SIP_TRANS, fname), ccb->index,
                            cc_config_ipaddr_str, cc_config_port,
                            cc_remote_ipaddr_str, cc_remote_port);

        if (conn_type != NULL) {
            if (!cpr_strcasecmp(conn_type, "UDP")) {
                if (util_compare_ip(&cc_config_ipaddr, cc_remote_ipaddr) &&
                    (cc_config_port == cc_remote_port)) {
                    send_to_proxy_handle = sipTransportGetServerHandle(ccb->dn_line,
                                                                       ccb->index);
                    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Got handle %d\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                     send_to_proxy_handle);
                }
            } else { 
                if (util_compare_ip(&cc_config_ipaddr, cc_remote_ipaddr)) {
                    send_to_proxy_handle = sipTransportGetServerHandle(ccb->dn_line,
                                                                       ccb->index);
                    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Got handle %d\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                     send_to_proxy_handle);
                    if (send_to_proxy_handle == INVALID_SOCKET) {
                        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Invalid socket\n", fname);
                        return (-1);
                    }
                }
            }
        } else {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "Invalid Connection type returned");
            return (-1);
        }
    }

    





    config_get_value(CFGID_NAT_ENABLE, &nat_enable, sizeof(nat_enable));
    if (nat_enable == 1) {
        send_to_proxy_handle = INVALID_SOCKET;
        config_get_value(CFGID_VOIP_CONTROL_PORT, &local_udp_port,
                         sizeof(local_udp_port));
    }
    








    if (ccb) {
        if (ccb->outBoundProxyPort == 0) {
            sipTransportGetOutbProxyAddress(ccb->dn_line, obp_address);
            if ((cpr_strcasecmp(obp_address, UNPROVISIONED) != 0) &&
                (obp_address[0] != 0) && (obp_address[0] != '0')) {

                
                dnsErrorCode = sipTransportGetServerAddrPort(obp_address,
                                         &ccb->outBoundProxyAddr,
                                         (uint16_t *)&ccb->outBoundProxyPort,
                                         &ccb->ObpSRVhandle,
                                         TRUE);

                if (dnsErrorCode != DNS_OK) {
                    



                    if (util_check_if_ip_valid(&(ccb->outBoundProxyAddr)) == FALSE) {
                        dnsErrorCode = dnsGetHostByName(obp_address,
                                                         &ccb->outBoundProxyAddr,
                                                         100, 1);
                    }
                    if (dnsErrorCode == DNS_OK) {
                        util_ntohl(&(ccb->outBoundProxyAddr),&(ccb->outBoundProxyAddr));
                    } else {
                        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                          "sipTransportSendMessage",
                                          "dnsGetHostByName() returned error");

                        ccb->outBoundProxyAddr = ip_addr_invalid;
                        ccb->outBoundProxyPort = 0;
                        return (-1);
                    }
                } else {
                    util_ntohl(&(ccb->outBoundProxyAddr), &(ccb->outBoundProxyAddr));
                }
            }
        }

        




        if (util_check_if_ip_valid(&(ccb->outBoundProxyAddr)) && 
            (ccb->proxySelection == SIP_PROXY_DEFAULT) &&
            (ccb->routeMode != RouteEmergency) && (ccb->index != REG_BACKUP_CCB)) {
            send_to_proxy_handle = INVALID_SOCKET;
            switch (message_type) {
            case sipMethodResponse:
            case sipMethodUnknown:
                ccb->outBoundProxyPort = cc_remote_port;
                break;
            default:
                




                *cc_remote_ipaddr = ccb->outBoundProxyAddr;
                if (ccb->outBoundProxyPort != 0) {
                    cc_remote_port = (uint16_t) ccb->outBoundProxyPort;
                } else {
                    cc_remote_port = (uint16_t) sipTransportGetOutbProxyPort(ccb->dn_line);
                    ccb->outBoundProxyPort = cc_remote_port;
                }
                break;
            }
        }
    }

    if (SipDebugTask || SipDebugMessage) {
        ipaddr2dotted(cc_remote_ipaddr_str, cc_remote_ipaddr);
    }

    if ((conn_type != NULL) && (cpr_strcasecmp(conn_type, "UDP")) &&
        (send_to_proxy_handle == INVALID_SOCKET)) {
        send_to_proxy_handle = sipTransportGetServerHandleWithAddr(cc_remote_ipaddr);
        CCSIP_DEBUG_TASK(DEB_F_PREFIX"<%s> remote ip addr\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), cc_remote_ipaddr_str);
        if (send_to_proxy_handle == INVALID_SOCKET) {
            
            
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"No connection to ip addr <%s>\n", fname, cc_remote_ipaddr_str);
            return (-1);
        }
    }
    if (send_to_proxy_handle != INVALID_SOCKET) {
        








        if (!cpr_strcasecmp(conn_type, "UDP")) {
            if (sip_platform_udp_channel_sendto(send_to_proxy_handle,
                                                pOutMessageBuf,
                                                nbytes,
                                                cc_remote_ipaddr,
                                                cc_remote_port) == SIP_ERROR) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                  fname, "sip_platform_udp_channel_sendto()");
                return (-1);
            }
        }
        else {
            


            tcp_error = sip_tcp_channel_send(send_to_proxy_handle,
                                     pOutMessageBuf,
                                     (unsigned short)nbytes);
            if (tcp_error == SIP_TCP_SEND_ERROR) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                  fname, "sip_platform_tcp_channel_send()");
                return (-1);
            }
        }

        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Sent SIP message: handle=<%d>,"
                            "length=<%d>, message=\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                            send_to_proxy_handle, nbytes);
        CCSIP_DEBUG_MESSAGE_PKT(pOutMessageBuf);
    } else {
        
        cpr_socket_t one_time_handle = INVALID_SOCKET;
        int status = -1;

        
        status = sip_platform_udp_channel_create(ip_mode, &one_time_handle,
                                                 cc_remote_ipaddr,
                                                 cc_remote_port,
                                                 local_udp_port);
        if (status < 0) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sip_platform_udp_channel_create()");
            return (-1);
        }
        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Opened a one-time UDP send channel to server "
                            "<%s>:<%d>, handle = %d local port= %d\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                            cc_remote_ipaddr_str, cc_remote_port,
                            one_time_handle, local_udp_port);

        
        if (sip_platform_udp_channel_sendto(one_time_handle,
                                            pOutMessageBuf,
                                            nbytes,
                                            cc_remote_ipaddr,
                                            cc_remote_port) == SIP_ERROR) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sip_platform_udp_channel_sendto()");
            
            status = sip_platform_udp_channel_destroy(one_time_handle);
            if (status < 0) {
                CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                  fname, "sip_platform_udp_channel_destroy()");
            }
            return (-1);
        }
        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Sent SIP message to <%s>:<%d>, "
                            "handle=<%d>, length=<%d>, message=\n",
                            DEB_F_PREFIX_ARGS(SIP_TRANS, fname), cc_remote_ipaddr_str, cc_remote_port,
                            one_time_handle, nbytes);
        CCSIP_DEBUG_MESSAGE_PKT(pOutMessageBuf);

        
        status = sip_platform_udp_channel_destroy(one_time_handle);
        if (status < 0) {
            CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                              fname, "sip_platform_udp_channel_destroy()");
            return (-1);
        }
        CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Closed a one-time UDP send channel "
                            "handle = %d\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), one_time_handle);
    }

    if (ccb) {
        
        
        
        


        if ((ccb->index <= REG_BACKUP_CCB) && reTx) {
            CCSIP_DEBUG_STATE(get_debug_string(DEBUG_SIP_ENTRY),
                              ccb->index, ccb->dn_line, fname,
                              "Stopping reTx timer");
            sip_platform_msg_timer_stop(ccb->index);

            







            if ((timeout > 0) && ((!cpr_strcasecmp(conn_type, "UDP")
                || ((tcp_error == CPR_ENOTCONN) && (!cpr_strcasecmp(conn_type, "TCP")))))) {
                void *data;

                data = isRegister ? (void *) ccb : (void *)(long)ccb->index;

                CCSIP_DEBUG_STATE(DEB_F_PREFIX"LINE %d/%d: Starting reTx timer (%d "
                                  "msec)\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), ccb->index, ccb->dn_line,
                                  timeout);
                ccb->retx_flag = TRUE;
                if (sip_platform_msg_timer_start(timeout, data, ccb->index,
                                                 pOutMessageBuf, nbytes,
                                                 (int) message_type, cc_remote_ipaddr,
                                                 cc_remote_port, isRegister) != SIP_OK) {
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_SIP_FUNCTIONCALL_FAILED), ccb->index,
                                      ccb->dn_line, fname,
                                      "sip_platform_msg_timer_start()");
                    ccb->retx_flag = FALSE;
                }
            }
        }

        if (sipMethodCancel == message_type || sipMethodBye == message_type) {
            gCallHistory[ccb->index].last_bye_dest_ipaddr = *cc_remote_ipaddr;
            gCallHistory[ccb->index].last_bye_dest_port   = cc_remote_port;
        }
    }
    else {
        sipSCB_t *scbptr = (sipSCB_t *)cbp;
        ccsip_publish_cb_t *pcb_p = (ccsip_publish_cb_t *)cbp;
        sipTCB_t *tcbp = (sipTCB_t *)cbp;

        if (cbp != NULL) {
            sipPlatformUITimer_t *timer = NULL;
            uint32_t id = 0;

            scbptr->hb.retx_flag = TRUE;
            if (((ccsip_common_cb_t *)cbp)->cb_type == SUBNOT_CB) {
                timer = &(sipPlatformUISMSubNotTimers[scbptr->line]);
                id = scbptr->line;
            } else if (((ccsip_common_cb_t *)cbp)->cb_type == PUBLISH_CB) {
                timer = &(pcb_p->retry_timer);
                id = pcb_p->pub_handle;
            } else { 
                int temp_timeout = 0;
                config_get_value(CFGID_TIMER_T1, &temp_timeout, sizeof(temp_timeout));
                temp_timeout = (64 * temp_timeout);
                if (cprStartTimer(tcbp->timer, temp_timeout, (void *)(long)(tcbp->trxn_id)) == CPR_FAILURE) {
                    CCSIP_DEBUG_STATE(DEB_F_PREFIX"%s failed\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname), "cprStartTimer");
                }
            }
 
            if (timeout > 0 && timer != NULL) {
                CCSIP_DEBUG_STATE(DEB_F_PREFIX"Starting reTx timer for %d secs",
                                  DEB_F_PREFIX_ARGS(SIP_TRANS, fname), timeout);
                if (sip_platform_msg_timer_subnot_start(timeout, timer, id,
                                                        pOutMessageBuf, nbytes,
                                                        (int) message_type,
                                                        cc_remote_ipaddr,
                                                        cc_remote_port) != SIP_OK) {
                    CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                                      fname, "sip_platform_msg_timer_subnot_start");
                }
            }
        }

    }

    return (0);
}















uint16_t
sipTransportGetListenPort (line_t line, ccsipCCB_t *ccb)
{
    ti_config_table_t *ccm_table_ptr = NULL;
    static const char *fname = "sipTransportGetListenPort";

    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return 0;
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        if (ccb) {
            ccm_table_ptr = (ti_config_table_t *) ccb->cc_cfg_table_entry;
        }
        if (ccm_table_ptr) {
            CCM_ID ccm_id;

            ccm_id = ccm_table_ptr->ti_specific.ti_ccm.ccm_id;
            if (ccm_id >= MAX_CCM) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"ccm id <%d> out of bounds.\n",
                                  fname, ccm_id);
                return 0;
            }
            return ((uint16_t) CCM_Config_Table[line - 1][ccm_id]->
                    ti_common.listen_port);
        } else if (CCM_Active_Standby_Table.active_ccm_entry != NULL) {
            return ((uint16_t) CCM_Active_Standby_Table.active_ccm_entry->
                    ti_common.listen_port);
        } else {
            return ((uint16_t) CCM_Config_Table[line - 1][PRIMARY_CCM]->
                    ti_common.listen_port);
        }
    } else {
        


        return ((uint16_t) CSPS_Config_Table[line - 1].ti_common.listen_port);
    }
}















const char *
sipTransportGetTransportType (line_t line, boolean upper_case,
                              ccsipCCB_t *ccb)
{
    const char *tcp, *udp, *tls;
    CONN_TYPE conn_type;
    ti_config_table_t *ccm_table_ptr = NULL;
    static const char *fname = "sipTransportGetTransportType";

    tcp = (upper_case) ? "TCP" : "tcp";
    udp = (upper_case) ? "UDP" : "udp";
    tls = (upper_case) ? "TLS" : "tls";

    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return udp;
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        
        
        
        if (ccb) {
            ccm_table_ptr = (ti_config_table_t *) ccb->cc_cfg_table_entry;
        }
        if (ccm_table_ptr) {
            conn_type = ccm_table_ptr->ti_common.conn_type;
        } else if (CCM_Active_Standby_Table.active_ccm_entry != NULL) {
            conn_type = CCM_Active_Standby_Table.active_ccm_entry->ti_common.conn_type;
        } else {
            conn_type = CCM_Device_Specific_Config_Table[PRIMARY_CCM].ti_common.conn_type;
        }
    } else {
        


        conn_type = CSPS_Config_Table[line - 1].ti_common.conn_type;
    }
    switch (conn_type) {
    case CONN_UDP:
        return (udp);
    case CONN_TCP:
    case CONN_TCP_TMP:
        return (tcp);
    case CONN_TLS:
        return (tls);
    default:
        return (NULL);
    }
}

















int
sipTransportGetServerAddrPort (char *domain, cpr_ip_addr_t *ipaddr_ptr,
                               uint16_t *port, srv_handle_t *psrv_order,
                               boolean retried_addr)
{
    int rc;

    if (psrv_order == NULL) {
        rc = sip_dns_gethostbysrvorname(domain, ipaddr_ptr, port);
    } else {
        rc = sip_dns_gethostbysrv(domain, ipaddr_ptr, port, psrv_order,
                                  retried_addr);
    }
    return (rc);
}















int
sipTransportGetPrimServerPort (line_t line)
{
    static const char *fname = "sipTransportGetPrimServerPort";
    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return (0);
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        if (CCM_Active_Standby_Table.active_ccm_entry != NULL) {
            return (CCM_Active_Standby_Table.active_ccm_entry->
                    ti_common.port);
        } else {
            return (0);
        }
    } else {
        


        return (CSPS_Config_Table[line - 1].ti_common.port);
    }
}















int
sipTransportGetBkupServerPort (line_t line)
{
    static const char *fname = "sipTransportGetBkupServerPort";
    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return (0);
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        return (0);
    } else {
        


        ti_csps_t *ti_csps;

        ti_csps = CSPS_Config_Table[line - 1].ti_specific.ti_csps;
        return (ti_csps->bkup_pxy_port);
    }
}















int
sipTransportGetEmerServerPort (line_t line)
{
    static const char *fname = "sipTransportGetEmerServerPort";
    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return (0);
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        return (0);
    } else {
        


        ti_csps_t *ti_csps;

        ti_csps = CSPS_Config_Table[line - 1].ti_specific.ti_csps;
        return (ti_csps->emer_pxy_port);
    }
}















int
sipTransportGetOutbProxyPort (line_t line)
{
    static const char *fname = "sipTransportGetOutbProxyPort";
    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return (0);
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        return (0);
    } else {
        


        ti_csps_t *ti_csps;

        ti_csps = CSPS_Config_Table[line - 1].ti_specific.ti_csps;
        return (ti_csps->outb_pxy_port);
    }
}















cpr_ip_type
sipTransportGetPrimServerAddress (line_t line, char *buffer)
{
    ti_common_t *ti_common;
    cpr_ip_type ip_type = CPR_IP_ADDR_IPV4;
    static const char *fname = "sipTransportGetPrimServerAddress";

    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return (ip_type);
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        if (CCM_Active_Standby_Table.active_ccm_entry != NULL) {
            sstrncpy(buffer, CCM_Active_Standby_Table.active_ccm_entry->
                     ti_common.addr_str, MAX_IPADDR_STR_LEN);
            ip_type = CCM_Active_Standby_Table.active_ccm_entry->ti_common.addr.type;

        } else {
            ti_common = &CCM_Device_Specific_Config_Table[PRIMARY_CCM].ti_common;
            sstrncpy(buffer, ti_common->addr_str, MAX_IPADDR_STR_LEN);
            ip_type = ti_common->addr.type;
        }
    } else {
        


        ti_common = &CSPS_Config_Table[line - 1].ti_common;
        sstrncpy(buffer, ti_common->addr_str, MAX_IPADDR_STR_LEN);
        ip_type = ti_common->addr.type;
    }

    return(ip_type);
}
















uint16_t
sipTransportGetBkupServerAddress (cpr_ip_addr_t *pip_addr,
                                  line_t line, char *buffer)
{
    static const char *fname = "sipTransportGetBkupServerAddress";
    *pip_addr = ip_addr_invalid;

    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return (0);
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        



        sstrncpy(buffer, "UNPROVISIONED", MAX_IPADDR_STR_LEN);
        return (0);
    } else {
        


        ti_csps_t *ti_csps;

        ti_csps = CSPS_Config_Table[line - 1].ti_specific.ti_csps;
        sstrncpy(buffer, ti_csps->bkup_pxy_addr_str, MAX_IPADDR_STR_LEN);
        *pip_addr = ti_csps->bkup_pxy_addr;
        return (1);
    }
}















void
sipTransportGetEmerServerAddress (line_t line, char *buffer)
{
    static const char *fname = "sipTransportGetEmerServerAddress";
    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return;
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        sstrncpy(buffer, "UNPROVISIONED", MAX_IPADDR_STR_LEN);
    } else {
        


        ti_csps_t *ti_csps;

        ti_csps = CSPS_Config_Table[line - 1].ti_specific.ti_csps;
        sstrncpy(buffer, ti_csps->emer_pxy_addr_str, MAX_IPADDR_STR_LEN);
    }
}















void
sipTransportGetOutbProxyAddress (line_t line, char *buffer)
{
    static const char *fname = "sipTransportGetOutbProxyAddress";
    



    if (((int)line < 1) || ((int)line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, line);
        return;
    }

    if (CC_Config_Table[line - 1].cc_type == CC_CCM) {
        


        sstrncpy(buffer, "UNPROVISIONED", MAX_IPADDR_STR_LEN);
    } else {
        


        ti_csps_t *ti_csps;

        ti_csps = CSPS_Config_Table[line - 1].ti_specific.ti_csps;
        sstrncpy(buffer, ti_csps->outb_pxy_addr_str, MAX_IPADDR_STR_LEN);
    }
}











void
sipTransportGetServerIPAddr (cpr_ip_addr_t *pip_addr, line_t line)
{
    const char     *fname = "sipTransportGetServerIPAddr";
    cpr_ip_addr_t   IPAddress;
    uint16_t        port;
    srv_handle_t     srv_order = NULL;
    int             dnsErrorCode = 0;
    char            addr[MAX_IPADDR_STR_LEN];
    char            obp_address[MAX_IPADDR_STR_LEN];

    CPR_IP_ADDR_INIT(IPAddress);

    sipTransportGetOutbProxyAddress(line, obp_address);
    if ((cpr_strcasecmp(obp_address, UNPROVISIONED) != 0) &&
        (obp_address[0] != 0) && (obp_address[0] != '0')) {
        sstrncpy(addr, obp_address, MAX_IPADDR_STR_LEN);
    } else {
        sipTransportGetPrimServerAddress(line, addr);
    }
    dnsErrorCode = sipTransportGetServerAddrPort(addr, &IPAddress, &port,
                                                 &srv_order, FALSE);
    if (srv_order) {
        dnsFreeSrvHandle(srv_order);
    }

    if (dnsErrorCode != DNS_OK) {
        
        
        dnsErrorCode = dnsGetHostByName(addr, &IPAddress, 100, 1);
    }

    if (dnsErrorCode != 0) {
        CCSIP_DEBUG_ERROR(get_debug_string(DEBUG_GENERAL_FUNCTIONCALL_FAILED),
                          fname, "dnsGetHostByName()");
    }
    *pip_addr = IPAddress;
    util_ntohl(pip_addr, &IPAddress);
}













void
sip_regmgr_set_cc_info (line_t line, line_t dn_line,
                        CC_ID *cc_type, void *cc_table_entry)
{
    static const char *fname = "sip_regmgr_set_cc_info";
    ti_config_table_t **active_standby_table_entry =
        (ti_config_table_t **) cc_table_entry;

    



    if (((int)dn_line < 1) || ((int)dn_line > MAX_REG_LINES)) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"Args check: DN <%d> out of bounds.\n",
                          fname, dn_line);
        return;
    }

    *cc_type = CC_Config_Table[dn_line - 1].cc_type;
    if (*cc_type == CC_CCM) {
        if (line == REG_BACKUP_CCB) {
            *active_standby_table_entry =
                CCM_Active_Standby_Table.standby_ccm_entry;
        } else {
            *active_standby_table_entry =
                CCM_Active_Standby_Table.active_ccm_entry;
        }
    }
}

int
sipTransportGetCCType (int line, void *cc_table_entry)
{
    if (cc_table_entry != NULL) {
        cc_table_entry = CC_Config_Table[line - 1].cc_table_entry;
    }
    return (CC_Config_Table[line - 1].cc_type);
}












int
SIPTransportUDPListenForSipMessages (void)
{
    static const char *fname = "SIPTransportUDPListenForSipMessages";
    uint32_t local_sip_control_port;
    cpr_ip_addr_t    local_sip_ip_addr;
    int            ip_mode = CPR_IP_MODE_IPV4;

    


    CPR_IP_ADDR_INIT(local_sip_ip_addr);

    config_get_value(CFGID_VOIP_CONTROL_PORT, &local_sip_control_port,
                     sizeof(local_sip_control_port));

#ifdef IPV6_STACK_ENABLED
   config_get_value(CFGID_IP_ADDR_MODE, &ip_mode, sizeof(ip_mode));
#endif
    switch (ip_mode) {
    case CPR_IP_MODE_IPV4:
        local_sip_ip_addr.type = CPR_IP_ADDR_IPV4;
        local_sip_ip_addr.u.ip4 = 0;
        break;
    case CPR_IP_MODE_IPV6:
    case CPR_IP_MODE_DUAL:
        local_sip_ip_addr = ip_addr_invalid;
        local_sip_ip_addr.type = CPR_IP_ADDR_IPV6;
        break;
    default:
        break;
    }
    


    if (sip_platform_udp_channel_listen(ip_mode, &listen_socket, &local_sip_ip_addr,
                                        (uint16_t) local_sip_control_port)
            != SIP_OK) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sip_platform_udp_channel_listen(0, %d) "
                          "returned error.\n", fname, local_sip_control_port);
        return SIP_ERROR;
    }

    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Listening for SIP messages on UDP port <%d>, handle=<%d>\n",
                     DEB_F_PREFIX_ARGS(SIP_TRANS, fname), local_sip_control_port, listen_socket);

    return SIP_OK;
}

static void
sipTransportCfgTableInit (boolean *cc_udp)
{

    line_t line;
    CC_ID dev_cc_type = CC_OTHER;
    uint32_t transport_prot = CONN_UDP;
    ti_common_t *ti_common;
    static const char *fname = "sipTransportCfgTableInit";


    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"Transport Interface init\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname));

    ti_common = &CSPS_Config_Table[0].ti_common;
    sip_config_get_proxy_addr(1, ti_common->addr_str, sizeof(ti_common->addr_str));

    if (!cpr_strcasecmp(ti_common->addr_str, "USECALLMANAGER")) {
        dev_cc_type = CC_CCM;
    }
    if (dev_cc_type == CC_CCM) {
        


        uint32_t listen_port;
        CCM_ID ccm_id;
        ti_ccm_t *ti_ccm;

        memset(CCM_Config_Table, 0,
               (sizeof(uint32_t) * MAX_CCM * (MAX_REG_LINES + 1)));
        config_get_value(CFGID_VOIP_CONTROL_PORT, &listen_port,
                         sizeof(listen_port));
        config_get_value(CFGID_TRANSPORT_LAYER_PROT, &transport_prot,
                         sizeof(transport_prot));
        if (transport_prot != CONN_UDP) {
            *cc_udp = FALSE;
        }

        



        CCM_Dummy_Entry.cc_type = CC_CCM;
        CCM_Dummy_Entry.ti_specific.ti_ccm.ccm_id = MAX_CCM;
        CCM_Dummy_Entry.ti_common.conn_type = (CONN_TYPE) transport_prot;

        for (ccm_id = PRIMARY_CCM; ccm_id < MAX_CCM; ccm_id++) {
            uint32_t port;
            phone_local_tcp_port[ccm_id] = 0;
            ti_common = &CCM_Device_Specific_Config_Table[ccm_id].ti_common;
            ti_ccm = &CCM_Device_Specific_Config_Table[ccm_id].ti_specific.ti_ccm;

            CCM_Device_Specific_Config_Table[ccm_id].cc_type = CC_CCM;
            sip_regmgr_get_config_addr(ccm_id, ti_common->addr_str);

            config_get_value(ccm_config_id_port[ccm_id], &port, sizeof(port));
            ti_common->port = (uint16_t) port;
            ti_common->conn_type = ti_common->configured_conn_type = (CONN_TYPE) transport_prot;
            ti_common->listen_port = (uint16_t) listen_port;
            ti_common->handle = INVALID_SOCKET;
            


            ti_ccm->ccm_id = (CCM_ID) ccm_id;
            ti_ccm->sec_level = NON_SECURE;
            ti_ccm->is_valid = 1;
            config_get_value(ccm_config_id_sec_level[ccm_id],
                             &ti_ccm->sec_level, sizeof(ti_ccm->sec_level));
            config_get_value(ccm_config_id_is_valid[ccm_id],
                             &ti_ccm->is_valid, sizeof(ti_ccm->is_valid));
            if ((ti_ccm->sec_level == NON_SECURE) &&
                (transport_prot == CONN_TLS)) {
                ti_common->conn_type = CONN_TCP;
            }
            for (line = 0; line < MAX_REG_LINES; line++) {
                CCM_Config_Table[line][ccm_id] =
                    &CCM_Device_Specific_Config_Table[ccm_id];
                if (ccm_id == PRIMARY_CCM) {
                    CC_Config_Table[line].cc_type = CC_CCM;
                    CC_Config_Table[line].cc_table_entry = (void *)
                        CCM_Config_Table[ccm_id];
                }
            }
            CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"For CCM%d: line %d Addr: %s Port: %d"
                                " listen Port: %d transport: %d"
                                " Sec Level: %d Is Valid: %d\n",
                                DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                ccm_id, line, ti_common->addr_str,
                                ti_common->port, ti_common->listen_port,
                                ti_common->conn_type, ti_ccm->sec_level,
                                ti_ccm->is_valid);
        }
    } else {
        ti_csps_t *ti_csps = &CSPS_Device_Specific_Config_Table;
        uint32_t bkup_pxy_port;
        uint32_t emer_pxy_port;
        uint32_t outb_pxy_port;
        uint32_t listen_port;

        sip_config_get_backup_proxy_addr(&(ti_csps->bkup_pxy_addr),
                                     ti_csps->bkup_pxy_addr_str,
                                     sizeof(ti_csps->bkup_pxy_addr_str));
        config_get_value(CFGID_PROXY_BACKUP_PORT, &bkup_pxy_port,
                         sizeof(bkup_pxy_port));
        ti_csps->bkup_pxy_port = (uint16_t) bkup_pxy_port;
        config_get_string(CFGID_PROXY_EMERGENCY, ti_csps->emer_pxy_addr_str,
                          sizeof(ti_csps->emer_pxy_addr_str));
        config_get_value(CFGID_PROXY_EMERGENCY_PORT, &emer_pxy_port,
                         sizeof(emer_pxy_port));
        ti_csps->emer_pxy_port = (uint16_t) emer_pxy_port;
        config_get_string(CFGID_OUTBOUND_PROXY, ti_csps->outb_pxy_addr_str,
                          sizeof(ti_csps->outb_pxy_addr_str));
        config_get_value(CFGID_OUTBOUND_PROXY_PORT, &outb_pxy_port,
                         sizeof(outb_pxy_port));
        ti_csps->outb_pxy_port = (uint16_t) outb_pxy_port;

        config_get_value(CFGID_VOIP_CONTROL_PORT, &listen_port,
                         sizeof(listen_port));
        for (line = 0; line < MAX_REG_LINES; line++) {
            ti_common = &CSPS_Config_Table[line].ti_common;
            CSPS_Config_Table[line].ti_specific.ti_csps = ti_csps;

            sip_config_get_proxy_addr((line_t)(line + 1), ti_common->addr_str,
                                      sizeof(ti_common->addr_str));
            ti_common->port = sip_config_get_proxy_port((line_t) (line + 1));
            ti_common->conn_type = CONN_UDP;
            ti_common->listen_port = (uint16_t) listen_port;
            ti_common->addr = ip_addr_invalid;
            ti_common->handle = INVALID_SOCKET;

            CC_Config_Table[line].cc_table_entry = (void *) NULL; 
            CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"line %d Addr: %s Port: %d and listen Port: %d\n"
                                " transport: %d\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                line, ti_common->addr_str, ti_common->port,
                                ti_common->listen_port, ti_common->conn_type);
            if (line == 0) {
                ti_csps_t *ti_csps_cfg_table;

                ti_csps_cfg_table = CSPS_Config_Table[line].ti_specific.ti_csps;
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"bkup Addr: %s and Port: %d\n",
                                    DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                    ti_csps_cfg_table->bkup_pxy_addr_str,
                                    ti_csps_cfg_table->bkup_pxy_port);
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"emer Addr: %s and Port: %d\n",
                                    DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                    ti_csps_cfg_table->emer_pxy_addr_str,
                                    ti_csps_cfg_table->emer_pxy_port);
                CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX"outb Addr: %s and Port: %d\n",
                                    DEB_F_PREFIX_ARGS(SIP_TRANS, fname),
                                    ti_csps_cfg_table->outb_pxy_addr_str,
                                    ti_csps_cfg_table->outb_pxy_port);
            }
        }
    }
}

















int
sipTransportInit (void)
{
    int result = 0;
    static const char *fname = "sipTransportInit";
    boolean cc_udp = TRUE;

    CCSIP_DEBUG_TASK(DEB_F_PREFIX"Transport_interface: Init function "
                     "call !\n", DEB_F_PREFIX_ARGS(SIP_TRANS, fname));
    


    sipTransportCfgTableInit(&cc_udp);
    


    if (PHNGetState() > STATE_IP_CFG) {
        if (cc_udp) {
            





            if (SIPTransportUDPListenForSipMessages() == SIP_ERROR) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"device unable to"
                                  " receive SIP messages.\n", fname);
            }
        } else {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"CCM in non udp mode so not "
                             "opening separate listen socket.\n",DEB_F_PREFIX_ARGS(SIP_TRANS, fname));
        }
        if (sip_regmgr_init() != SIP_OK) {
            result = SIP_ERROR;
        }
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"IP Stack Not "
                         "Initialized.\n", fname);
        result = -1;
    }
    return (result);
}













void
sipTransportShutdown ()
{
    CCSIP_DEBUG_STATE(DEB_F_PREFIX"Transport_interface: Shutting down!\n", DEB_F_PREFIX_ARGS(SIP_TRANS, "sipTransportShutdown"));
    sip_regmgr_destroy_cc_conns();
}













void
sipTransportClearServerHandle (cpr_ip_addr_t *ipaddr, uint16_t port, int connid)
{

    ti_common_t *ti_common;
    CCM_ID cc_index;

    CCSIP_DEBUG_TASK(DEB_F_PREFIX"addr 0x%x port %d connid %d\n", 
                     DEB_F_PREFIX_ARGS(SIP_TRANS, "sipTransportClearServerHandle"), ipaddr, port, connid);
    for (cc_index = PRIMARY_CCM; cc_index < MAX_CCM; cc_index++) {
        ti_common = &CCM_Device_Specific_Config_Table[cc_index].ti_common;
        if (util_compare_ip(&(ti_common->addr),ipaddr) && ti_common->port == port) {
            sip_tcp_purge_entry(connid);
            ti_common->handle = INVALID_SOCKET;
            ti_common->listen_port = 0;
            return;
        }
    }
}













void
sipTransportSetServerHandleAndPort (cpr_socket_t socket_handle,
                                    uint16_t listen_port,
                                    ti_config_table_t *ccm_table_entry)
{
    ti_common_t *ti_common;
    ti_ccm_t *ti_ccm;

    ti_ccm = &ccm_table_entry->ti_specific.ti_ccm;

    if (ti_ccm->ccm_id < MAX_CCM) {
        ti_common = &CCM_Device_Specific_Config_Table[ti_ccm->ccm_id].ti_common;
        ti_common->handle = socket_handle;
        ti_common->listen_port = listen_port;
    }
}

void
sipTransportSetSIPServer() {
	char addr_str[MAX_IPADDR_STR_LEN];
	init_empty_str(addr_str);
	config_get_string(CFGID_CCM1_ADDRESS, addr_str, MAX_IPADDR_STR_LEN);
	sstrncpy(CCM_Config_Table[0][0]->ti_common.addr_str, addr_str, MAX_IPADDR_STR_LEN);
	sstrncpy(CCM_Device_Specific_Config_Table[PRIMARY_CCM].ti_common.addr_str, addr_str, MAX_IPADDR_STR_LEN);
}
