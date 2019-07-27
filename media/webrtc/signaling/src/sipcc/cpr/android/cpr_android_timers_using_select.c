


















































#include "cpr.h"
#include "cpr_assert.h"
#include "cpr_socket.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "cpr_threads.h"
#include "cpr_timers.h"
#include "cpr_string.h"
#include "phntask.h"
#include <errno.h>
#include <unistd.h>
#include "cpr_android_timers.h"
#include "platform_api.h"






typedef struct timer_ipc_cmd_s
{
    cpr_timer_t *timer_ptr;
    void        *user_data_ptr;
    uint32_t    duration;
} timer_ipc_cmd_t;


typedef struct timer_ipc_s
{
    uint32_t  msg_type;
    union
    {
        timer_ipc_cmd_t cmd;
        cprRC_t         result;
    }u;
} timer_ipc_t;

#define TMR_CMD_ADD    1
#define TMR_CMD_REMOVE 2
#define TMR_RESULT     3

#define API_RETURN(_val) \
    {                     \
        pthread_mutex_unlock(&api_mutex); \
        return (_val); \
     }\

#define API_ENTER() \
{\
    pthread_mutex_lock(&api_mutex);\
}\




#define SERVER_PATH "/tmp/CprTmrServer"
#define CLIENT_PATH "/tmp/CprTmrClient"








static timerBlk *timerListHead;

static pthread_t timerThreadId;


static pthread_mutex_t api_mutex;



static int client_sock = INVALID_SOCKET;



static int serv_sock = INVALID_SOCKET;

static struct sockaddr_un tmr_serv_addr;
static struct sockaddr_un tmr_client_addr;

static fd_set socks; 



















extern void fillInSysHeader(void *buffer, uint16_t cmd, uint16_t len,
                            void *timerMsg);








static void     *timerThread(void *data);
static cprRC_t  start_timer_service_loop();
static void     process_expired_timers();
static void     send_api_result(cprRC_t result, struct sockaddr_un *addr, socklen_t len);



















void
cprSleep (uint32_t duration)
{
    



    if (duration >= 1000) {
        (void) sleep(duration / 1000);
        (void) usleep((duration % 1000) * 1000);
    } else {
        (void) usleep(duration * 1000);
    }
}























static cprRC_t addTimerToList (cpr_timer_t *cprTimerPtr, uint32_t duration, void *data)
{
#ifdef CPR_TIMERS_ENABLED
    timer_ipc_t tmr_cmd = {0};
    timer_ipc_t tmr_rsp={0};

    API_ENTER();

    
    
    tmr_cmd.msg_type = TMR_CMD_ADD;
    tmr_cmd.u.cmd.timer_ptr = cprTimerPtr;
    tmr_cmd.u.cmd.user_data_ptr = data;
    tmr_cmd.u.cmd.duration = duration;


    
    if (client_sock != -1) {
        if (sendto(client_sock, &tmr_cmd, sizeof(timer_ipc_t), 0,
                   (struct sockaddr *)&tmr_serv_addr, sizeof(tmr_serv_addr)) < 0) {
            CPR_ERROR("Failed to tx IPC msg to timer service, errno = %s %s\n",
                   strerror(errno), __FUNCTION__);
            API_RETURN(CPR_FAILURE);
        }

    } else {
        CPR_ERROR("can not make IPC connection, client_sock is invalid %s\n", __FUNCTION__);
        API_RETURN(CPR_FAILURE);
    }

    




    if (recvfrom(client_sock, &tmr_rsp, sizeof(timer_ipc_t),0, NULL, NULL) < 0) {
        
        API_RETURN(CPR_FAILURE);
    } else {
        
        API_RETURN(tmr_rsp.u.result);
    }
#else
    cprAssert(FALSE, CPR_FAILURE);
    CPR_ERROR("CPR Timers are disabled! %s\n", __FUNCTION__);
    return CPR_SUCCESS;
#endif
}















static cprRC_t addTimer (cpr_timer_t *cprTimerPtr, uint32_t duration, void *data)
{
    static const char fname[] = "addTimer";
    timerBlk *timerList;
    timerBlk *newTimerPtr;

    CPR_INFO("%s:adding timer=0x%x timerblk=%x\n", fname,
           cprTimerPtr, cprTimerPtr->u.handlePtr);


    
    newTimerPtr = (timerBlk *) cprTimerPtr->u.handlePtr;
    if (newTimerPtr == NULL) {
        CPR_ERROR("%s - Timer %s has not been initialized.\n",
                  fname, cprTimerPtr->name);
        errno = EINVAL;

        return(CPR_FAILURE);
    }

    
    if (newTimerPtr->timerActive) {
        CPR_ERROR("%s - Timer %s is already active.\n", fname, cprTimerPtr->name);
        errno = EAGAIN;
        return(CPR_FAILURE);

    }

    
    newTimerPtr->duration = duration;
    cprTimerPtr->data = data;

    










    
    if (timerListHead == NULL) {
        
        timerListHead = newTimerPtr;
    } else {

	
        timerList = timerListHead;
        while (timerList != NULL) {

            






            if (newTimerPtr->duration < timerList->duration) {
                
                timerList->duration -= newTimerPtr->duration;
                newTimerPtr->next = timerList;
                newTimerPtr->previous = timerList->previous;
                if (newTimerPtr->previous) {
                    newTimerPtr->previous->next = newTimerPtr;
                }
                timerList->previous = newTimerPtr;

                
                if (timerListHead == timerList) {
                    
                    timerListHead = newTimerPtr;
                }
                break;
            } else {
                







                
                newTimerPtr->duration -= timerList->duration;

                
                if (timerList->next == NULL) {
                    
                    newTimerPtr->previous = timerList;
                    timerList->next = newTimerPtr;
                    newTimerPtr->next = NULL;
                    break;
                }
                timerList = timerList->next;
            }
        }
    }

    newTimerPtr->timerActive = TRUE;
    return(CPR_SUCCESS);
}














static cprRC_t
removeTimerFromList (cpr_timer_t *cprTimerPtr)
{

    static const char fname[] = "removeTimerFromList";
    timer_ipc_t tmr_cmd = {0};
    timer_ipc_t tmr_rsp = {0};


    API_ENTER();

    
    tmr_cmd.msg_type = TMR_CMD_REMOVE;
    tmr_cmd.u.cmd.timer_ptr = cprTimerPtr;

    

    
    if (client_sock != -1) {
        if (sendto(client_sock, &tmr_cmd, sizeof(timer_ipc_t), 0,
                   (struct sockaddr *)&tmr_serv_addr, sizeof(tmr_serv_addr)) < 0) {
            CPR_ERROR("%s:failed to tx IPC Msg to timer service, errno = %s\n",
                      fname, strerror(errno));
            API_RETURN(CPR_FAILURE);
        }
    } else {
        CPR_ERROR("%s:client_sock invalid, no IPC connection \n", fname);
        API_RETURN(CPR_FAILURE);
    }

    




    if (recvfrom(client_sock, &tmr_rsp, sizeof(timer_ipc_t),0, NULL, NULL) < 0) {
        
        API_RETURN(CPR_FAILURE);
    } else {
        
        API_RETURN(tmr_rsp.u.result);
    }
}












static cprRC_t
removeTimer (cpr_timer_t *cprTimerPtr)
{
    static const char fname[] = "removeTimer";
    timerBlk *timerList;
    timerBlk *previousTimer;
    timerBlk *nextTimer;
    timerBlk *timerPtr;

    

    






    timerPtr = (timerBlk *) cprTimerPtr->u.handlePtr;
    

    if (timerPtr != NULL) {
        
        timerList = timerListHead;
        while (timerList != NULL) {
            if (timerList->cprTimerPtr->cprTimerId == cprTimerPtr->cprTimerId) {
                
                if ((timerList->previous == NULL) &&
                    (timerList->next == NULL)) {
                    timerListHead = NULL;

                
                } else if (timerList->previous == NULL) {
                    nextTimer = timerList->next;
                    nextTimer->previous = NULL;
                    timerListHead = nextTimer;

                
                } else if (timerList->next == NULL) {
                    previousTimer = timerList->previous;
                    previousTimer->next = NULL;

                
                } else {
                    nextTimer = timerList->next;
                    previousTimer = timerList->previous;
                    previousTimer->next = nextTimer;
                    nextTimer->previous = previousTimer;
                }

                
                if (timerList->next) {
                    timerList->next->duration += timerList->duration;
                }

                


                timerList->next = NULL;
                timerList->previous = NULL;
                timerList->duration = -1;
                timerList->timerActive = FALSE;
                cprTimerPtr->data = NULL;

                return(CPR_SUCCESS);

            }

            
            timerList = timerList->next;
        }

        




        if ((timerPtr->next != NULL) || (timerPtr->previous != NULL)) {
            CPR_ERROR("%s - Timer %s marked as active, "
                      "but was not found on the timer list.\n",
                      fname, cprTimerPtr->name);
            timerPtr->next = NULL;
            timerPtr->previous = NULL;
        }
        timerPtr->duration = -1;
        timerPtr->cprTimerPtr->data = NULL;
        timerPtr->timerActive = FALSE;

        return(CPR_SUCCESS);

    }

    
    CPR_ERROR("%s - Timer not initialized.\n", fname);
    errno = EINVAL;
    return(CPR_FAILURE);

}






























cprTimer_t
cprCreateTimer (const char *name,
                uint16_t applicationTimerId,
                uint16_t applicationMsgId,
                cprMsgQueue_t callBackMsgQueue)
{
    static const char fname[] = "cprCreateTimer";
    static uint32_t cprTimerId = 0;
    cpr_timer_t *cprTimerPtr;
    timerBlk *timerPtr;

    




    cprTimerPtr = (cpr_timer_t *) cpr_malloc(sizeof(cpr_timer_t));
    timerPtr = (timerBlk *) cpr_malloc(sizeof(timerBlk));
    if ((cprTimerPtr != NULL) && (timerPtr != NULL)) {
        
        cprTimerPtr->name = name;

        
        cprTimerPtr->applicationTimerId = applicationTimerId;
        cprTimerPtr->applicationMsgId = applicationMsgId;
        cprTimerPtr->cprTimerId = cprTimerId++;
        if (callBackMsgQueue == NULL) {
            CPR_ERROR("%s - Callback msg queue for timer %s is NULL.\n",
                      fname, name);
            cpr_free(timerPtr);
            cpr_free(cprTimerPtr);
            return NULL;
        }
        cprTimerPtr->callBackMsgQueue = callBackMsgQueue;

        


        timerPtr->next = NULL;
        timerPtr->previous = NULL;
        timerPtr->duration = -1;
        timerPtr->timerActive = FALSE;
        cprTimerPtr->data = NULL;

        












        timerPtr->cprTimerPtr = cprTimerPtr;
        cprTimerPtr->u.handlePtr = timerPtr;
        

        return cprTimerPtr;
    }

    


    if (timerPtr) {
        cpr_free(timerPtr);
    }
    if (cprTimerPtr) {
        cpr_free(cprTimerPtr);
    }

    
    CPR_ERROR("%s - Malloc for timer %s failed.\n", fname, name);
    errno = ENOMEM;
    return NULL;
}


















cprRC_t
cprStartTimer (cprTimer_t timer,
               uint32_t duration,
               void *data)
{
    static const char fname[] = "cprStartTimer";
    cpr_timer_t *cprTimerPtr;

    cprTimerPtr = (cpr_timer_t *) timer;
    if (cprTimerPtr != NULL) {
        
        return addTimerToList(cprTimerPtr, duration, data);
    }

    
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}















boolean
cprIsTimerRunning (cprTimer_t timer)
{
    static const char fname[] = "cprIsTimerRunning";
    cpr_timer_t *cprTimerPtr;
    timerBlk *timerPtr;

    

    cprTimerPtr = (cpr_timer_t *) timer;
    if (cprTimerPtr != NULL) {
        timerPtr = (timerBlk *) cprTimerPtr->u.handlePtr;
        if (timerPtr == NULL) {
            CPR_ERROR("%s - Timer %s has not been initialized.\n",
                      fname, cprTimerPtr->name);
            errno = EINVAL;
            return FALSE;
        }

        if (timerPtr->timerActive) {
            return TRUE;
        }
    } else {
        
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        errno = EINVAL;
    }

    return FALSE;
}














cprRC_t
cprCancelTimer (cprTimer_t timer)
{
    static const char fname[] = "cprCancelTimer";
    timerBlk *timerPtr;
    cpr_timer_t *cprTimerPtr;
    cprRC_t rc = CPR_SUCCESS;

    

    cprTimerPtr = (cpr_timer_t *) timer;
    if (cprTimerPtr != NULL) {
        timerPtr = (timerBlk *) cprTimerPtr->u.handlePtr;
        if (timerPtr == NULL) {
            CPR_ERROR("%s - Timer %s has not been initialized.\n",
                      fname, cprTimerPtr->name);
            errno = EINVAL;
            return CPR_FAILURE;
        }

        



        if (timerPtr->timerActive) {
            
            rc = removeTimerFromList(timer);
        }
        return rc;
    }

    
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}
















cprRC_t
cprUpdateTimer (cprTimer_t timer, uint32_t duration)
{
    static const char fname[] = "cprUpdateTimer";
    cpr_timer_t *cprTimerPtr;
    void *timerData;

    cprTimerPtr = (cpr_timer_t *) timer;
    if (cprTimerPtr != NULL) {
        
        timerData = cprTimerPtr->data;
    } else {
        CPR_ERROR("%s - NULL pointer passed in.\n", fname);
        errno = EINVAL;
        return CPR_FAILURE;
    }

    if (cprCancelTimer(timer) == CPR_SUCCESS) {
        if (cprStartTimer(timer, duration, timerData) == CPR_SUCCESS) {
            return CPR_SUCCESS;
        } else {
            CPR_ERROR("%s - Failed to start timer %s\n",
                      fname, cprTimerPtr->name);
            return CPR_FAILURE;
        }
    }

    CPR_ERROR("%s - Failed to cancel timer %s\n", fname, cprTimerPtr->name);
    return CPR_FAILURE;
}














cprRC_t
cprDestroyTimer (cprTimer_t timer)
{
    static const char fname[] = "cprDestroyTimer";
    cpr_timer_t *cprTimerPtr;
    cprRC_t rc;

    

    cprTimerPtr = (cpr_timer_t *) timer;
    if (cprTimerPtr != NULL) {
        rc = cprCancelTimer(timer);
        if (rc == CPR_SUCCESS) {
            cprTimerPtr->cprTimerId = 0;
            cpr_free(cprTimerPtr->u.handlePtr);
            cpr_free(cprTimerPtr);
            return CPR_SUCCESS;
        } else {
            CPR_ERROR("%s - Cancel of Timer %s failed.\n",
                      fname, cprTimerPtr->name);
            return CPR_FAILURE;
        }
    }

    
    CPR_ERROR("%s - NULL pointer passed in.\n", fname);
    errno = EINVAL;
    return CPR_FAILURE;
}















cprRC_t cpr_timer_pre_init (void)
{
    static const char fname[] = "cpr_timer_pre_init";
    int32_t returnCode;

    
    returnCode = (int32_t)pthread_create(&timerThreadId, NULL, timerThread, NULL);
    if (returnCode == -1) {
        CPR_ERROR("%s: Failed to create Timer Thread : %s\n", fname, strerror(errno));
        return CPR_FAILURE;
    }

    



    cprSleep(1000);

    return CPR_SUCCESS;
}









cprRC_t cpr_timer_de_init(void)
{
    
    close(client_sock);
    close(serv_sock);


    
    pthread_mutex_destroy(&api_mutex);

    return CPR_SUCCESS;
}

















void *timerThread (void *data)
{
    static const char fname[] = "timerThread";

    
#ifndef HOST
#ifndef PTHREAD_SET_NAME
#define PTHREAD_SET_NAME(s)     do { } while (0)
#endif
    PTHREAD_SET_NAME("CPR Timertask");
#endif

    






    (void) cprAdjustRelativeThreadPriority(TIMER_THREAD_RELATIVE_PRIORITY);

    
    if (start_timer_service_loop() == CPR_FAILURE) {
        CPR_ERROR("%s: timer service loop failed\n", fname);
    }

    return NULL;
}











static int local_bind (int sock, char *name)
{
    struct sockaddr_un addr;

    
    addr.sun_family = AF_LOCAL;
    sstrncpy(addr.sun_path, name, sizeof(addr.sun_path));
    
    unlink(addr.sun_path);

    return bind(sock, (struct sockaddr *) &addr, sizeof(addr));
}









static int select_sockets (void)
{
    FD_ZERO(&socks);

    FD_SET(serv_sock, &socks);

    return (serv_sock);
}









static cprRC_t read_timer_cmd ()
{
    static const char fname[] = "read_timer_cmd";
    int  rcvlen;
    timer_ipc_t tmr_cmd ={0};
    cprRC_t ret = CPR_FAILURE;



    rcvlen =recvfrom(serv_sock, &tmr_cmd, sizeof(timer_ipc_t), 0,
                     NULL, NULL);

    if (rcvlen > 0) {
        
        switch(tmr_cmd.msg_type) {
	case TMR_CMD_ADD:
            
            

            ret = addTimer(tmr_cmd.u.cmd.timer_ptr,tmr_cmd.u.cmd.duration,
                     (void *)tmr_cmd.u.cmd.user_data_ptr);

            break;

	case TMR_CMD_REMOVE:
            
            ret = removeTimer(tmr_cmd.u.cmd.timer_ptr);
            break;

        default:
            CPR_ERROR("%s:invalid ipc command = %d\n", tmr_cmd.msg_type);
            ret = CPR_FAILURE;
            break;
        }
    } else {
        CPR_ERROR("%s:while reading serv_sock err =%s: Closing Socket..Timers not operational !!! \n",
                  fname, strerror(errno));
        (void) close(serv_sock);
        serv_sock = INVALID_SOCKET;
        ret = CPR_FAILURE;
    }

    
    send_api_result(ret, &tmr_client_addr, sizeof(tmr_client_addr));

    return (ret);

}







void send_api_result(cprRC_t retVal, struct sockaddr_un *addr, socklen_t len)
{
    static const char fname[] = "send_api_result";
    timer_ipc_t tmr_rsp = {0};

    tmr_rsp.msg_type = TMR_RESULT;
    tmr_rsp.u.result = retVal;
    if (sendto(serv_sock, &tmr_rsp, sizeof(timer_ipc_t),0, (struct sockaddr *)addr, len) < 0) {
        CPR_ERROR("%s: error in sending on serv_sock err=%s\n", fname, strerror(errno));
    }
}









cprRC_t start_timer_service_loop (void)
{
    static const char fname[] = "start_timer_service_loop";
    int lsock = -1;
    struct timeval tv;
    int ret;
    boolean use_timeout;


    
    cpr_set_sockun_addr((cpr_sockaddr_un_t *) &tmr_serv_addr,   SERVER_PATH, getpid());
    cpr_set_sockun_addr((cpr_sockaddr_un_t *) &tmr_client_addr, CLIENT_PATH, getpid());

    



    if (pthread_mutex_init(&api_mutex, NULL) != 0) {
        CPR_ERROR("%s: failed to initialize api_mutex err=%s\n", fname,
                  strerror(errno));
        return CPR_FAILURE;
    }


    
    client_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (client_sock == INVALID_SOCKET) {
        CPR_ERROR("%s:could not create client socket error=%s\n", fname, strerror(errno));
        return CPR_FAILURE;
    }

    
    if (local_bind(client_sock,tmr_client_addr.sun_path) < 0) {
        CPR_ERROR("%s:could not bind local socket:error=%s\n", fname, strerror(errno));
        (void) close(client_sock);
        client_sock = INVALID_SOCKET;
        return CPR_FAILURE;
    }

    
    serv_sock = socket(AF_LOCAL, SOCK_DGRAM, 0);
    if (serv_sock == INVALID_SOCKET) {
        CPR_ERROR("%s:could not create server socket error=%s\n", fname, strerror(errno));
        serv_sock = INVALID_SOCKET;
        close(client_sock);
        client_sock = INVALID_SOCKET;
        return CPR_FAILURE;
    }

    if (local_bind(serv_sock, tmr_serv_addr.sun_path) < 0) {
        CPR_ERROR("%s:could not bind serv socket:error=%s\n", fname, strerror(errno));
        (void) close(serv_sock);
        (void) close(client_sock);
        client_sock = serv_sock = INVALID_SOCKET;
        return CPR_FAILURE;
    }


    while (1) {

        lsock = select_sockets();

	
	if (timerListHead != NULL) {
            tv.tv_sec = (timerListHead->duration)/1000;
            tv.tv_usec = (timerListHead->duration%1000)*1000;
            
            
            
            use_timeout = TRUE;
	} else {
            
            
            use_timeout = FALSE;
	}

        ret = select(lsock + 1, &socks, NULL, NULL, (use_timeout == TRUE) ? &tv:NULL);

        if (ret == -1) {
            CPR_ERROR("%s:error in select err=%s\n", fname,
                      strerror(errno));
            return(CPR_FAILURE);
        } else if (ret == 0) {
            


            timerListHead->duration = 0;
            process_expired_timers();
        } else {

            if (FD_ISSET(serv_sock, &socks)) {
                
                
                if (timerListHead != NULL) {
                    
                    
                    
                    
                    timerListHead->duration = tv.tv_sec * 1000 + (tv.tv_usec/1000);
                }
                
                (void) read_timer_cmd();
            }
	}
    }

}









void process_expired_timers() {
}




