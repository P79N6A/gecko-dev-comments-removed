



#include "cpr_types.h"
#include "cpr_ipc.h"
#include "cpr_errno.h"
#include "cpr_socket.h"
#include "cpr_in.h"
#include "ccsip_core.h"
#include "ccsip_task.h"
#include "sip_platform_task.h"
#include "ccsip_platform_udp.h"
#include "sip_common_transport.h"
#include "sip_common_regmgr.h"
#include "phone_debug.h"
#include "util_string.h"
#include "ccsip_platform_tcp.h"
#include "text_strings.h"
#include "ccsip_register.h"
#include "phntask.h"
#include "plat_api.h"
#include "sip_socket_api.h"

#define HOST_SIZE 64


cpr_socket_t sip_tls_create_connection(sipSPIMessage_t *spi_msg,
                                       boolean blocking,
                                       sec_level_t sec);
extern cpr_sockaddr_t *sip_set_sockaddr(cpr_sockaddr_storage *psock_storage, uint16_t family, 
                                 cpr_ip_addr_t ip_addr, uint16_t port, uint16_t *addr_len);












cpr_socket_t
sip_tls_create_connection (sipSPIMessage_t *spi_msg, boolean blocking,
        sec_level_t sec)
{
    const char fname[] = "sip_tls_create_connection";
    int idx;
    sipSPICreateConnection_t *create_msg;
    char ipaddr_str[MAX_IPADDR_STR_LEN];
    plat_soc_status_e ret;
    cpr_socket_t sock = INVALID_SOCKET;
    uint16_t sec_port = 0;
    plat_soc_connect_status_e conn_status;
    int tos_dscp_val = 0; 
#ifdef IPV6_STACK_ENABLED
    int ip_mode = CPR_IP_MODE_IPV4;
#endif
    uint16_t af_listen = AF_INET6;
    cpr_sockaddr_storage sock_addr;
    uint16_t       addr_len;
    int ip_mode = 0; 
    plat_soc_connect_mode_e conn_mode;

#ifdef IPV6_STACK_ENABLED

    config_get_value(CFGID_IP_ADDR_MODE, &ip_mode, sizeof(ip_mode));

    


    if (ip_mode == CPR_IP_MODE_IPV6 ||
        ip_mode == CPR_IP_MODE_DUAL) {
        af_listen = AF_INET6;
    } else {
#endif
        af_listen = AF_INET;
#ifdef IPV6_STACK_ENABLED
    }
#endif

    sip_tcp_init_conn_table();
    create_msg = &(spi_msg->createConnMsg);
    ipaddr2dotted(ipaddr_str, &create_msg->addr);
    ret = platSecIsServerSecure();
    if (ret != PLAT_SOCK_SECURE) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX 
                          "Secure connection is not created because"
                          " there is no secure servers\n", fname);
        return INVALID_SOCKET;
    }
    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX
                        "Creating secure connection\n", DEB_F_PREFIX_ARGS(SIP_TLS, fname));
    
    config_get_value(CFGID_DSCP_FOR_CALL_CONTROL, (int *)&tos_dscp_val,
                     sizeof(tos_dscp_val));

    if (sec == AUTHENTICATED) {
        conn_mode = PLAT_SOCK_AUTHENTICATED;
    } else if (sec == ENCRYPTED) {
        conn_mode = PLAT_SOCK_ENCRYPTED;
    } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX 
                          "Secure connection is not created. Security mode was"
                          " not encrypyted or authenticated.\n", fname);
        conn_mode = PLAT_SOCK_NON_SECURE;
    }
    sock = platSecSocConnect(ipaddr_str,          
                            create_msg->port,    
                            ip_mode,             
                            blocking,            
                            tos_dscp_val, 
                            conn_mode,   
                            &sec_port);          
    
    if (sock < 0) {

        CCSIP_DEBUG_ERROR(SIP_F_PREFIX 
                          "Secure connect failed!!\n",fname);
        return INVALID_SOCKET;
    }
    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX "Secure connect ok\n", DEB_F_PREFIX_ARGS(SIP_TLS, fname));
    if (!blocking) {
        
        conn_status = platSecSockIsConnected(sock);
        if (conn_status == PLAT_SOCK_CONN_FAILED) {
            (void)sipSocketClose(sock, TRUE);
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX 
                              "Establish non-blocking mode secure"
                              " connection failed!!\n", fname);
            return INVALID_SOCKET;
        }
    } else {
        conn_status = PLAT_SOCK_CONN_OK;   
    }
    if (sip_tcp_set_sock_options(sock) != TRUE) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX "Socket set option failure\n",
                          fname);
    }

    idx = sip_tcp_get_free_conn_entry();
    if (idx == -1) {
        
        (void)sipSocketClose(sock, TRUE);
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX
                          "Get free TCP connection entry failed\n",
                           fname);
        return INVALID_SOCKET;
    }

    memset(&sock_addr, 0, sizeof(sock_addr));
    (void) sip_set_sockaddr(&sock_addr, af_listen, create_msg->addr, 
                            (uint16_t)(create_msg->port), &addr_len);

    sip_tcp_conn_tab[idx].fd = sock;
    sip_tcp_conn_tab[idx].ipaddr = create_msg->addr;
    sip_tcp_conn_tab[idx].port = create_msg->port;
    sip_tcp_conn_tab[idx].context = spi_msg->context;
    sip_tcp_conn_tab[idx].dirtyFlag = FALSE;
    sip_tcp_conn_tab[idx].addr = sock_addr;
    sip_tcp_conn_tab[idx].soc_type = SIP_SOC_TLS;

    if (conn_status == PLAT_SOCK_CONN_OK) {
        sip_tcp_conn_tab[idx].state = SOCK_CONNECTED;
    } else {
        sip_tcp_conn_tab[idx].state = SOCK_CONNECT_PENDING;
    }
    create_msg->local_listener_port = (uint16) sec_port;
    CCSIP_DEBUG_MESSAGE(DEB_F_PREFIX
                        "Local listening port=%d\n", DEB_F_PREFIX_ARGS(SIP_TLS, fname),
                        create_msg->local_listener_port);
    (void)sip_tcp_attach_socket(sock);
    return (sock);
}
