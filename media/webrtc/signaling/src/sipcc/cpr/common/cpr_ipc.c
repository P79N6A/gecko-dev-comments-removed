



#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "plat_api.h"
#include "cpr_string.h"

#ifdef SIP_OS_WINDOWS
#include <windows.h>
#include <process.h>
#include <winuser.h>
#else
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#endif 


#ifdef SIP_OS_WINDOWS
extern cprMsgQueue_t sip_msgq;
extern cprMsgQueue_t gsm_msgq;
extern cprMsgQueue_t tmr_msgq;

extern void gsm_shutdown();
extern void sip_shutdown();






static char rcvBuffer[100];
#define MSG_BUF 0xF000

#else

#define OS_MSGTQL 31 /* need to check number for MV linux and put here */




extern pthread_t cprGetThreadId(cprThread_t thread);







typedef struct cpr_msgq_node_s
{
    struct cpr_msgq_node_s *next;
    struct cpr_msgq_node_s *prev;
    void *msg;
    void *pUserData;
} cpr_msgq_node_t;














typedef struct cpr_msg_queue_s
{
    struct cpr_msg_queue_s *next;
    const char *name;
    pthread_t thread;
    int32_t queueId;
    uint16_t currentCount;
    uint32_t totalCount;
    uint32_t sendErrors;
    uint32_t reTries;
    uint32_t highAttempts;
    uint32_t selfQErrors;
    uint16_t extendedQDepth;
    uint16_t maxExtendedQDepth;
    pthread_mutex_t mutex;       
    pthread_cond_t cond;         
    cpr_msgq_node_t *head;       
    cpr_msgq_node_t *tail;       
} cpr_msg_queue_t;





typedef enum
{
    CPR_MSGQ_POST_SUCCESS,
    CPR_MSGQ_POST_FAILED,
    CPR_MSGQ_POST_PENDING
} cpr_msgq_post_result_e;





static cpr_msg_queue_t *msgQueueList = NULL;




pthread_mutex_t msgQueueListMutex;











#define CPR_MAX_MSG_Q_DEPTH 256











#define CPR_SND_TIMEOUT_WAIT_INTERVAL 20












#define CPR_ATTEMPTS_TO_SEND 25






















static void
cprPegSendMessageStats (cpr_msg_queue_t *msgq, uint16_t numAttempts)
{
    


    msgq->totalCount++;

    if (numAttempts > msgq->highAttempts) {
        msgq->highAttempts = numAttempts;
    }
}














static cpr_msgq_post_result_e
cprPostMessage (cpr_msg_queue_t *msgq, void *msg, void **ppUserData)
{
    cpr_msgq_node_t *node;

    


    node = cpr_malloc(sizeof(*node));
    if (!node) {
        errno = ENOMEM;
        return CPR_MSGQ_POST_FAILED;
    }

    pthread_mutex_lock(&msgq->mutex);

    


    node->msg = msg;
    if (ppUserData != NULL) {
        node->pUserData = *ppUserData;
    } else {
        node->pUserData = NULL;
    }

    


    node->prev = NULL;
    node->next = msgq->head;
    msgq->head = node;

    if (node->next) {
        node->next->prev = node;
    }

    if (msgq->tail == NULL) {
        msgq->tail = node;
    }
    msgq->currentCount++;

    pthread_cond_signal(&msgq->cond);
    pthread_mutex_unlock(&msgq->mutex);

    return CPR_MSGQ_POST_SUCCESS;

}
#endif 




















cprMsgQueue_t
cprCreateMessageQueue (const char *name, uint16_t depth)
{
    cpr_msg_queue_t *msgq;

#ifndef SIP_OS_WINDOWS
    static int key_id = 100; 
    pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
#endif

    msgq = cpr_calloc(1, sizeof(cpr_msg_queue_t));
    if (msgq == NULL) {
        printf("%s: Malloc failed: %s\n", __FUNCTION__,
               name ? name : "unnamed");
        errno = ENOMEM;
        return NULL;
    }

    msgq->name = name ? name : "unnamed";

#ifndef SIP_OS_WINDOWS
    msgq->queueId = key_id++;
    msgq->cond = _cond;
    msgq->mutex = _lock;

    


    pthread_mutex_lock(&msgQueueListMutex);
    msgq->next = msgQueueList;
    msgQueueList = msgq;
    pthread_mutex_unlock(&msgQueueListMutex);
#endif 

    return msgq;
}














cprRC_t
cprSetMessageQueueThread (cprMsgQueue_t msgQueue, cprThread_t thread)
{
    cpr_msg_queue_t *msgq;

    if ((!msgQueue) || (!thread)) {
        CPR_ERROR("%s: Invalid input\n", __FUNCTION__);
        return CPR_FAILURE;
    }

#ifdef SIP_OS_WINDOWS
    ((cpr_msg_queue_t *)msgQueue)->handlePtr = thread;
#else
    msgq = (cpr_msg_queue_t *) msgQueue;
    if (msgq->thread != 0) {
        CPR_ERROR("%s: over-writing previously msgq thread name for %s",
                  __FUNCTION__, msgq->name);
    }

    msgq->thread = cprGetThreadId(thread);
#endif 

    return CPR_SUCCESS;
}
















void *
cprGetMessage (cprMsgQueue_t msgQueue, boolean waitForever, void **ppUserData)
{
    void *buffer = NULL;

#ifdef SIP_OS_WINDOWS
    struct msgbuffer *rcvMsg = (struct msgbuffer *)rcvBuffer;
    cpr_msg_queue_t *pCprMsgQueue;
    MSG msg;
    cpr_thread_t *pThreadPtr;
#else
    cpr_msg_queue_t *msgq = (cpr_msg_queue_t *) msgQueue;
    cpr_msgq_node_t *node;
    struct timespec timeout;
    struct timeval tv;
    struct timezone tz;
#endif

    if (!msgQueue) {
        CPR_ERROR("%s - invalid msgQueue\n", __FUNCTION__);
        return NULL;
    }

    
    if (ppUserData) {
        *ppUserData = NULL;
    }

#ifdef SIP_OS_WINDOWS
    pCprMsgQueue = (cpr_msg_queue_t *)msgQueue;
    memset(&msg, 0, sizeof(MSG));

    if (waitForever == TRUE) {
        if (GetMessage(&msg, NULL, 0, 0) == -1) {
            CPR_ERROR("%s - msgQueue = %x failed: %d\n",
                      __FUNCTION__, msgQueue, GetLastError());
            return NULL;
        }
    } else {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == 0) {
            
            return NULL;
        }
    }

    switch (msg.message) {
        case WM_CLOSE:
            if (msgQueue == &gsm_msgq)
            {
                CPR_ERROR("%s - WM_CLOSE GSM msg queue\n", __FUNCTION__);
                gsm_shutdown();
            }
            else if (msgQueue == &sip_msgq)
            {
                CPR_ERROR("%s - WM_CLOSE SIP msg queue\n", __FUNCTION__);
                sip_regmgr_destroy_cc_conns();
                sip_shutdown();
            }
            else if (msgQueue == &tmr_msgq)
            {
                CPR_ERROR("%s - WM_CLOSE TMR msg queue\n", __FUNCTION__);
            }

            pThreadPtr=(cpr_thread_t *)pCprMsgQueue->handlePtr;
            if (pThreadPtr)
            {
                CloseHandle(pThreadPtr->u.handlePtr);
            }
            
            pCprMsgQueue->handlePtr = NULL;
            _endthreadex(0);
            break;
        case MSG_BUF:
            rcvMsg = (struct msgbuffer *)msg.wParam;
            buffer = rcvMsg->msgPtr;
            if (ppUserData) {
                *ppUserData = rcvMsg->usrPtr;
            }
            cpr_free((void *)msg.wParam);
            break;
        case MSG_ECHO_EVENT:
            {
                HANDLE event;
                event = (HANDLE*)msg.wParam;
                SetEvent( event );
            }
            break;
        case WM_TIMER:
            DispatchMessage(&msg);
            return NULL;
            break;
        default:
            break;
    }
#else
    




    pthread_mutex_lock(&msgq->mutex);

    if (!waitForever)
    {
        
        gettimeofday(&tv, &tz);
        timeout.tv_nsec = (tv.tv_usec * 1000) + 25000;
        timeout.tv_sec = tv.tv_sec;

        pthread_cond_timedwait(&msgq->cond, &msgq->mutex, &timeout);
    }
    else
    {
        while(msgq->tail==NULL)
        {
            pthread_cond_wait(&msgq->cond, &msgq->mutex);
        }
    }

    
    if (msgq->tail)
    {
        node = msgq->tail;
        msgq->tail = node->prev;
        if (msgq->tail) {
            msgq->tail->next = NULL;
        }
        if (msgq->head == node) {
            msgq->head = NULL;
        }
        msgq->currentCount--;
        


        if (ppUserData) {
            *ppUserData = node->pUserData;
        }
        buffer = node->msg;

    }

    pthread_mutex_unlock(&msgq->mutex);
#endif 

    return buffer;
}


































cprRC_t
cprSendMessage (cprMsgQueue_t msgQueue, void *msg, void **ppUserData)
{
#ifdef SIP_OS_WINDOWS
    struct msgbuffer *sendMsg;
    cpr_thread_t *pCprThread;
    HANDLE *hThread;
#else
    static const char error_str[] = "%s: Msg not sent to %s queue: %s\n";
    cpr_msgq_post_result_e rc;
    cpr_msg_queue_t *msgq = (cpr_msg_queue_t *) msgQueue;
    int16_t attemptsToSend = CPR_ATTEMPTS_TO_SEND;
    uint16_t numAttempts   = 0;
#endif

    if (!msgQueue) {
        CPR_ERROR("%s - msgQueue is NULL\n", __FUNCTION__);
        return CPR_FAILURE;
    }

#ifdef SIP_OS_WINDOWS
    pCprThread = (cpr_thread_t *)(((cpr_msg_queue_t *)msgQueue)->handlePtr);
    if (!pCprThread) {
        CPR_ERROR("%s - msgQueue(%x) not associated with a thread\n",
                  __FUNCTION__, msgQueue);
        return CPR_FAILURE;
    }

    hThread = (HANDLE*)(pCprThread->u.handlePtr);
    if (!hThread) {
        CPR_ERROR("%s - msgQueue(%x)'s thread(%x) not assoc. with Windows\n",
                __FUNCTION__, msgQueue, pCprThread);
        return CPR_FAILURE;
    }

    
    sendMsg = (struct msgbuffer *)cpr_calloc(1, sizeof(struct msgbuffer));
    if (!sendMsg) {
        CPR_ERROR("%s - No memory\n", __FUNCTION__);
        return CPR_FAILURE;
    }
    sendMsg->mtype = PHONE_IPC_MSG;

    
    sendMsg->msgPtr = msg;

    
    if (ppUserData) {
        sendMsg->usrPtr = *ppUserData;
    }

    
    if (hThread == NULL || PostThreadMessage(pCprThread->threadId, MSG_BUF,
        (WPARAM)sendMsg, 0) == 0 ) {
        CPR_ERROR("%s - Msg not sent: %d\n", __FUNCTION__, GetLastError());
        cpr_free(sendMsg);
        return CPR_FAILURE;
    }
    return CPR_SUCCESS;

#else
    


    do {

        


        rc = cprPostMessage(msgq, msg, ppUserData);

        if (rc == CPR_MSGQ_POST_SUCCESS) {
            cprPegSendMessageStats(msgq, numAttempts);
            return CPR_SUCCESS;
        } else if (rc == CPR_MSGQ_POST_FAILED) {
            CPR_ERROR("%s: Msg not sent to %s queue: %d\n",
                      __FUNCTION__, msgq->name, errno);
            msgq->sendErrors++;
            



            if (pthread_self() == msgq->thread) {
                msgq->selfQErrors++;
            }

            return CPR_FAILURE;
        }


        



        attemptsToSend--;
        if (attemptsToSend > 0) {
            







            cprSleep(CPR_SND_TIMEOUT_WAIT_INTERVAL);
            msgq->reTries++;
            numAttempts++;
        }
    } while (attemptsToSend > 0);

    CPR_ERROR(error_str, __FUNCTION__, msgq->name, "FULL");
    msgq->sendErrors++;
    return CPR_FAILURE;
#endif 
}

















uint16_t cprGetDepth (cprMsgQueue_t msgQueue)
{
#ifdef SIP_OS_WINDOWS
    return 0;
#else
    cpr_msg_queue_t *msgq;
    msgq = (cpr_msg_queue_t *) msgQueue;
    return msgq->currentCount;
#endif 
}


