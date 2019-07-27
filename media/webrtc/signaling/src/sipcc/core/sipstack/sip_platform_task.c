



#include "cpr_types.h"
#include "cpr_ipc.h"
#include "cpr_errno.h"
#include "cpr_socket.h"
#include "cpr_in.h"
#include "cpr_rand.h"
#include "cpr_string.h"
#include "cpr_threads.h"
#include "ccsip_core.h"
#include "ccsip_task.h"
#include "sip_platform_task.h"
#include "ccsip_platform_udp.h"
#include "sip_common_transport.h"
#include "phntask.h"
#include "phone_debug.h"
#include "util_string.h"
#include "ccsip_platform_tcp.h"
#include "ccsip_task.h"
#include "sip_socket_api.h"
#include "platform_api.h"
#include <sys/stat.h>
#include <stdio.h>
#include <unistd.h>
#include "prprf.h"








#define MAX_SIP_MESSAGES 8


#define MAX_SIP_CONNECTIONS (64 - 2)

#define SIP_PAUSE_WAIT_IPC_LISTEN_READY_TIME   50  /* 50ms. */
#define SIP_MAX_WAIT_FOR_IPC_LISTEN_READY    1200  /* 50 * 1200 = 1 minutes */






fd_set read_fds;
fd_set write_fds;
static cpr_socket_t listen_socket = INVALID_SOCKET;
static cpr_socket_t sip_ipc_serv_socket = INVALID_SOCKET;
static cpr_socket_t sip_ipc_clnt_socket = INVALID_SOCKET;
static boolean main_thread_ready = FALSE;
uint32_t nfds = 0;
sip_connection_t sip_conn;





typedef struct sip_int_msg_t_ {
    void            *msg;
    phn_syshdr_t    *syshdr;
} sip_int_msg_t;


static sip_int_msg_t sip_int_msgq_buf[MAX_SIP_MESSAGES] = {{0,0},{0,0}};







extern sipGlobal_t sip;
extern boolean  sip_reg_all_failed;



























static void
sip_platform_task_init (void)
{
    uint16_t i;

    for (i = 0; i < MAX_SIP_CONNECTIONS; i++) {
        sip_conn.read[i] = INVALID_SOCKET;
        sip_conn.write[i] = INVALID_SOCKET;
    }

    


    FD_ZERO(&read_fds);
    FD_ZERO(&write_fds);
    return;
}










static void sip_process_int_msg (void)
{
    const char    *fname = "sip_process_int_msg";
    ssize_t       rcv_len;
    uint8_t       num_messages = 0;
    uint8_t       response = 0;
    sip_int_msg_t *int_msg;
    void          *msg;
    phn_syshdr_t  *syshdr;

    
    rcv_len = cprRecv(sip_ipc_serv_socket, &num_messages,
		      sizeof(num_messages), 0);

    if (rcv_len < 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"read IPC failed:"
                          " errno=%d\n", fname, cpr_errno);
        return;
    }

    if (num_messages == 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"message queue is empty!", fname);
        return;
    }

    if (num_messages > MAX_SIP_MESSAGES) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"number of  messages on queue exceeds maximum %d", fname,
                          num_messages);
        num_messages = MAX_SIP_MESSAGES;
    }

    
    int_msg = &sip_int_msgq_buf[0];
    while (num_messages) {
        msg    = int_msg->msg;
        syshdr = int_msg->syshdr;
        if (msg != NULL && syshdr != NULL) {
            if (syshdr->Cmd == THREAD_UNLOAD) {
	      



                cprCloseSocket(sip_ipc_serv_socket);
            }
            SIPTaskProcessListEvent(syshdr->Cmd, msg, syshdr->Usr.UsrPtr,
                syshdr->Len);
            cprReleaseSysHeader(syshdr);

            int_msg->msg    = NULL;
            int_msg->syshdr = NULL;
        }

        num_messages--;  
        int_msg++;       
    }

    


    if (cprSend(sip_ipc_serv_socket, (void *)&response,
		sizeof(response), 0) < 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sending IPC", fname);
    }
}












void
sip_platform_task_set_listen_socket (cpr_socket_t s)
{
    listen_socket = s;
    sip_platform_task_set_read_socket(s);
}












void
sip_platform_task_set_read_socket (cpr_socket_t s)
{
#if 0
  
    if (s != INVALID_SOCKET) {
        FD_SET(s, &read_fds);
        nfds = MAX(nfds, (uint32_t)s);
    }
#endif
}












void
sip_platform_task_reset_listen_socket (cpr_socket_t s)
{
    sip_platform_task_clr_read_socket(s);
    listen_socket = INVALID_SOCKET;
}












void
sip_platform_task_clr_read_socket (cpr_socket_t s)
{
#if 0
  
    if (s != INVALID_SOCKET) {
        FD_CLR(s, &read_fds);
    }
#endif
}

