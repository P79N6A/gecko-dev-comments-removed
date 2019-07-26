






































#ifndef _SIP_PLATFORM_TASK_H_
#define _SIP_PLATFORM_TASK_H_

#include "cpr_socket.h"




void sip_platform_task_loop(void *arg);
void sip_platform_task_set_listen_socket(cpr_socket_t s);
void sip_platform_task_set_read_socket(cpr_socket_t s);
void sip_platform_task_clr_read_socket(cpr_socket_t s);

void sip_platform_task_reset_listen_socket(cpr_socket_t s);

#endif
