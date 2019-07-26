



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
#include "sip_interface_regmgr.h"
#include "phntask.h"
#include "phone_debug.h"
#include "util_string.h"
#include "ccsip_platform_tcp.h"
#include "ccsip_task.h"








#define MAX_SIP_MESSAGES 8


#define MAX_SIP_CONNECTIONS (64 - 2)


#define SIP_SELECT_NORMAL_TIMEOUT   25000    /* normal select timeout in usec*/
#define SIP_SELECT_QUICK_TIMEOUT        0    /* quick select timeout in usec */






fd_set read_fds;
fd_set write_fds;
static cpr_socket_t listen_socket = INVALID_SOCKET;
uint32_t nfds = 0;
sip_connection_t sip_conn;







extern sipGlobal_t sip;




























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













void
sip_platform_task_loop (void *arg)
{
    static const char *fname = "sip_platform_task_loop";
    int pending_operations;
    struct cpr_timeval timeout;
    void *msg;
    uint32_t cmd;
    uint16_t len;
    void *usr;
    phn_syshdr_t *syshdr;
    uint16_t i;
    fd_set sip_read_fds;


    sip_msgq = (cprMsgQueue_t) arg;
    if (!sip_msgq) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"sip_msgq is null, exiting\n", fname);
        return;
    }
    sip.msgQueue = sip_msgq;

    sip_platform_task_init();
    


    SIPTaskInit();

    if (platThreadInit("sip_platform_task_loop") != 0) {
        CCSIP_DEBUG_ERROR(SIP_F_PREFIX"failed to attach thread to JVM\n", fname);
        return;
    }

    


    (void) cprAdjustRelativeThreadPriority(SIP_THREAD_RELATIVE_PRIORITY);

    


    timeout.tv_sec  = 0;
    timeout.tv_usec = SIP_SELECT_NORMAL_TIMEOUT;

    





    cpr_srand((unsigned int)time(NULL));

    


    while (TRUE) {
        


        sip_read_fds = read_fds;


        pending_operations = cprSelect((nfds + 1),
                                       &sip_read_fds,
                                       NULL,
                                       NULL, &timeout);
        if (pending_operations == SOCKET_ERROR) {
            CCSIP_DEBUG_ERROR(SIP_F_PREFIX"cprSelect() failed: errno=%d\n",
                              fname, cpr_errno);
        } else if (pending_operations) {
            




            if ((listen_socket != INVALID_SOCKET) &&
                (sip.taskInited == TRUE) &&
                FD_ISSET(listen_socket, &sip_read_fds)) {
                sip_platform_udp_read_socket(listen_socket);
                pending_operations--;
            }
            


            for (i = 0; ((i < MAX_SIP_CONNECTIONS) &&
                         (pending_operations != 0)); i++) {
                if ((sip_conn.read[i] != INVALID_SOCKET) &&
                    FD_ISSET(sip_conn.read[i], &sip_read_fds)) {
                    


                    sip_tcp_read_socket(sip_conn.read[i]);
                    pending_operations--;
                }
				











            }
        }

        



        i = 0;
        while (i++ < MAX_SIP_MESSAGES) {
            msg = cprGetMessage(sip_msgq, FALSE, (void **) &syshdr);
            if (msg != NULL) {
                cmd = syshdr->Cmd;
                len = syshdr->Len;
                usr = syshdr->Usr.UsrPtr;
                SIPTaskProcessListEvent(cmd, msg, usr, len);
                cprReleaseSysHeader(syshdr);
                syshdr = NULL;
            } else {
                
                if (syshdr != NULL) {
                    cprReleaseSysHeader(syshdr);
                    syshdr = NULL;
                }
                break;
            }
        }

        





 
        if (i >= MAX_SIP_MESSAGES) {
            timeout.tv_usec = SIP_SELECT_QUICK_TIMEOUT;
        } else {
            
            timeout.tv_usec = SIP_SELECT_NORMAL_TIMEOUT;
        }
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
    if (s != INVALID_SOCKET) {
        FD_SET(s, &read_fds);
        nfds = MAX(nfds, (uint32_t)s);
    }
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
    if (s != INVALID_SOCKET) {
        FD_CLR(s, &read_fds);
    }
}

