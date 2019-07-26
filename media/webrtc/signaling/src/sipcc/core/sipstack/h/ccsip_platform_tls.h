






































#ifndef __CCSIP_PLATFORM_TLS__H__
#define __CCSIP_PLATFORM_TLS__H__

extern cpr_socket_t sip_tls_create_connection(sipSPIMessage_t *spi_msg,
                                              boolean blocking,
                                              sec_level_t sec);

#endif 
