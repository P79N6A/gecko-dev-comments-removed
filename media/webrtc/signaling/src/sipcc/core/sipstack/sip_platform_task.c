



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
#include "thread_monitor.h"








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


























void sip_platform_task_msgqwait (void *arg)
{
    const char    *fname = "sip_platform_task_msgqwait";
    cprMsgQueue_t *msgq = (cprMsgQueue_t *)arg;
    unsigned int  wait_main_thread = 0;
    phn_syshdr_t  *syshdr;
    void          *msg;
    uint8_t       num_messages = 0;
    uint8_t       response = 0;
    boolean       quit_thread = FALSE;

    if (msgq == NULL) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"task msgq is null, exiting", fname);
        return;
    }

    if (platThreadInit("SIP IPCQ task") != 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to attach thread to JVM", fname);
        return;
    }

    


    while (!main_thread_ready) {
        
        cprSleep(SIP_PAUSE_WAIT_IPC_LISTEN_READY_TIME);

        wait_main_thread++;
        if (wait_main_thread > SIP_MAX_WAIT_FOR_IPC_LISTEN_READY) {
            
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"timeout waiting for listening IPC"
                              " socket ready, exiting\n", fname);
            return;
        }
    }

    


    (void) cprAdjustRelativeThreadPriority(SIP_THREAD_RELATIVE_PRIORITY);

    while (quit_thread == FALSE) {
        msg = cprGetMessage(msgq, TRUE, (void **) &syshdr);
        while (msg != NULL) {
            



            sip_int_msgq_buf[num_messages].msg    = msg;
            sip_int_msgq_buf[num_messages].syshdr = syshdr;
            num_messages++;

            switch (syshdr->Cmd) {
            case THREAD_UNLOAD:
                thread_ended(THREADMON_MSGQ);
                quit_thread = TRUE;
                    break;
                default:
                    break;
            }

            if (num_messages == MAX_SIP_MESSAGES) {
                




                break;
            }
            





            msg = cprGetMessage(msgq, 0, (void **) &syshdr);
        }

        if (num_messages) {
            CCSIP_DEBUG_TASK(DEB_F_PREFIX"%d msg available on msgq", DEB_F_PREFIX_ARGS(SIP_MSG_QUE, fname), num_messages);
            



            if (cprSend(sip_ipc_clnt_socket, (void *)&num_messages,
			sizeof(num_messages), 0) < 0) {
                CCSIP_DEBUG_ERROR(SIP_F_PREFIX"send IPC failed errno=%d", fname, cpr_errno);
            }

            if (FALSE == quit_thread) {
            	


                if (cprRecv(sip_ipc_clnt_socket, &response,
			    sizeof(response), 0) < 0) {
            		CCSIP_DEBUG_ERROR(SIP_F_PREFIX"read IPC failed:"
            				" errno=%d\n", fname, cpr_errno);
            	}
            	num_messages = 0;
            }
        }
    }
    cprCloseSocket(sip_ipc_clnt_socket);
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
sip_platform_task_loop (void *arg)
{
    static const char *fname = "sip_platform_task_loop";
    int pending_operations;
    uint16_t i;
    fd_set sip_read_fds;
    fd_set sip_write_fds;
    sip_tcp_conn_t *entry;

    sip_msgq = (cprMsgQueue_t) arg;
    if (!sip_msgq) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sip_msgq is null, exiting", fname);
        return;
    }
    sip.msgQueue = sip_msgq;

    sip_platform_task_init();
    


    SIPTaskInit();

    if (platThreadInit("SIPStack Task") != 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to attach thread to JVM", fname);
        return;
    }

    


    (void) cprAdjustRelativeThreadPriority(SIP_THREAD_RELATIVE_PRIORITY);

    


    {
      cpr_socket_t sockets[2];
      if (cprSocketPair(AF_LOCAL, SOCK_DGRAM, 0, sockets) == CPR_SUCCESS) {
	sip_ipc_serv_socket = sockets[0];
	sip_ipc_clnt_socket = sockets[1];
      } else {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"socketpair failed:"
                          " errno=%d\n", fname, cpr_errno);
        return;
      }
    }

    





    cpr_srand((unsigned int)time(NULL));

    


    sip_platform_task_set_read_socket(sip_ipc_serv_socket);

    



    main_thread_ready = TRUE;

    



    while (TRUE) {
      sip_process_int_msg();

#if 0
       
        


        sip_read_fds = read_fds;

        
        FD_ZERO(&sip_write_fds);
        
        for (i = 0; i < MAX_CONNECTIONS; i++) {
            entry = sip_tcp_conn_tab + i;
            if (-1 != entry->fd && entry->sendQueue && sll_count(entry->sendQueue)) {
                FD_SET(entry->fd, &sip_write_fds);
            }
        }

        pending_operations = cprSelect((nfds + 1),
                                       &sip_read_fds,
                                       &sip_write_fds,
                                       NULL, NULL);
        if (pending_operations == SOCKET_ERROR) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"cprSelect() failed: errno=%d."
                " Recover by initiating sip restart\n",
                              fname, cpr_errno);
            







            sip_platform_task_init(); 
            sip_platform_task_set_read_socket(sip_ipc_serv_socket);

            







            sip_reg_all_failed = TRUE;
            platform_reset_req(DEVICE_RESTART);
            continue;
        } else if (pending_operations) {
            




            if ((listen_socket != INVALID_SOCKET) &&
                (sip.taskInited == TRUE) &&
                FD_ISSET(listen_socket, &sip_read_fds)) {
                sip_platform_udp_read_socket(listen_socket);
                pending_operations--;
            }

            


            if (FD_ISSET(sip_ipc_serv_socket, &sip_read_fds)) {
                
                sip_process_int_msg();
                pending_operations--;
            }

            


            for (i = 0; ((i < MAX_SIP_CONNECTIONS) &&
                         (pending_operations > 0)); i++) {
                if ((sip_conn.read[i] != INVALID_SOCKET) &&
                    FD_ISSET(sip_conn.read[i], &sip_read_fds)) {
                    


                    sip_tcp_read_socket(sip_conn.read[i]);
                    pending_operations--;
                }
                if ((sip_conn.write[i] != INVALID_SOCKET) &&
                    FD_ISSET(sip_conn.write[i], &sip_write_fds)) {
                    int connid;

                    connid = sip_tcp_fd_to_connid(sip_conn.write[i]);
                    if (connid >= 0) {
                        sip_tcp_resend(connid);
                    }
                    pending_operations--;
                }
            }
        }
#endif
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

