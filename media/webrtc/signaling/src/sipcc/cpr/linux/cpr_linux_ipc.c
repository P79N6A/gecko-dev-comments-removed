

































#include "cpr.h"
#include "cpr_stdlib.h"
#include <cpr_stdio.h>
#include <errno.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include "plat_api.h"
#include "CSFLog.h"

static const char *logTag = "cpr_linux_ipc";

#define STATIC static


#define OS_MSGTQL 31




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




static const char unnamed_string[] = "unnamed";












#define CPR_MAX_MSG_Q_DEPTH 256











#define CPR_SND_TIMEOUT_WAIT_INTERVAL 20












#define CPR_ATTEMPTS_TO_SEND 25















static cpr_msgq_post_result_e
cprPostMessage(cpr_msg_queue_t *msgq, void *msg, void **ppUserData);
static void
cprPegSendMessageStats(cpr_msg_queue_t *msgq, uint16_t numAttempts);
static cpr_msgq_post_result_e
cprPostExtendedQMsg(cpr_msg_queue_t *msgq, void *msg, void **ppUserData);
static void
cprMoveMsgToQueue(cpr_msg_queue_t *msgq);































cprMsgQueue_t
cprCreateMessageQueue (const char *name, uint16_t depth)
{
    static const char fname[] = "cprCreateMessageQueue";
    cpr_msg_queue_t *msgq;
    key_t key;
    static int key_id = 100; 
    struct msqid_ds buf;

    msgq =(cpr_msg_queue_t *)cpr_calloc(1, sizeof(cpr_msg_queue_t));
    if (msgq == NULL) {
        CPR_ERROR("%s: Malloc failed: %s\n", fname,
                  name ? name : unnamed_string);
        errno = ENOMEM;
        return NULL;
    }

    msgq->name = name ? name : unnamed_string;

    


    key = ftok("/proc/self", key_id++);
    CSFLogDebug(logTag, "key = %x\n", key);

    if (key == -1) {
        CPR_ERROR("%s: Key generation failed: %d\n", fname, errno);
        cpr_free(msgq);
        return NULL;
    }

    


    msgq->queueId = msgget(key, (IPC_EXCL | IPC_CREAT | 0666));
    if (msgq->queueId == -1) {
        if (errno == EEXIST) {
            CSFLogDebug(logTag, "Q exists so first remove it and then create again\n");
                
            msgq->queueId = msgget(key, (IPC_CREAT | 0666));
            if (msgctl(msgq->queueId, IPC_RMID, &buf) == -1) {

                CPR_ERROR("%s: Destruction failed: %s: %d\n", fname,
                          msgq->name, errno);

                return NULL;
            }
            msgq->queueId = msgget(key, (IPC_CREAT | 0666));
        }
    } else {
        CSFLogDebug(logTag, "there was no preexisting q..\n");

    }



    if (msgq->queueId == -1) {
        CPR_ERROR("%s: Creation failed: %s: %d\n", fname, name, errno);
        if (errno == EEXIST) {

        }

        cpr_free(msgq);
        return NULL;
    }
    CSFLogDebug(logTag, "create message q with id=%x\n", msgq->queueId);

    

    


    if (pthread_mutex_init(&msgq->mutex, NULL) != 0) {
        CPR_ERROR("%s: Failed to create msg queue (%s) mutex: %d\n",
                  fname, name, errno);
        (void) msgctl(msgq->queueId, IPC_RMID, &buf);
        cpr_free(msgq);
        return NULL;
    }

    


    if (depth > CPR_MAX_MSG_Q_DEPTH) {
        CPR_INFO("%s: Depth too large (%d) reset to %d\n", fname, depth,
                 CPR_MAX_MSG_Q_DEPTH);
        depth = CPR_MAX_MSG_Q_DEPTH;
    }

    if (depth < OS_MSGTQL) {
        if (depth) {
            CPR_INFO("%s: Depth too small (%d) reset to %d\n", fname, depth, OS_MSGTQL);
        }
        depth = OS_MSGTQL;
    }
    msgq->maxExtendedQDepth = depth - OS_MSGTQL;

    


    pthread_mutex_lock(&msgQueueListMutex);
    msgq->next = msgQueueList;
    msgQueueList = msgq;
    pthread_mutex_unlock(&msgQueueListMutex);

    return msgq;
}
















cprRC_t
cprDestroyMessageQueue (cprMsgQueue_t msgQueue)
{
    static const char fname[] = "cprDestroyMessageQueue";
    cpr_msg_queue_t *msgq;
    void *msg;
    struct msqid_ds buf;
    CSFLogDebug(logTag, "Destroy message Q called..\n");


    msgq = (cpr_msg_queue_t *) msgQueue;
    if (msgq == NULL) {
        
        CPR_ERROR("%s: Invalid input\n", fname);
        errno = EINVAL;
        return CPR_FAILURE;
    }

    
    msg = cprGetMessage(msgQueue, FALSE, NULL);
    while (msg != NULL) {
        cpr_free(msg);
        msg = cprGetMessage(msgQueue, FALSE, NULL);
    }

    
    pthread_mutex_lock(&msgQueueListMutex);
    if (msgq == msgQueueList) {
        msgQueueList = msgq->next;
    } else {
        cpr_msg_queue_t *msgql = msgQueueList;

        while ((msgql->next != NULL) && (msgql->next != msgq)) {
            msgql = msgql->next;
        }
        if (msgql->next == msgq) {
            msgql->next = msgq->next;
        }
    }
    pthread_mutex_unlock(&msgQueueListMutex);

    
    if (msgctl(msgq->queueId, IPC_RMID, &buf) == -1) {
        CPR_ERROR("%s: Destruction failed: %s: %d\n", fname,
                  msgq->name, errno);
        return CPR_FAILURE;
    }

    
    if (pthread_mutex_destroy(&msgq->mutex) != 0) {
        CPR_ERROR("%s: Failed to destroy msg queue (%s) mutex: %d\n",
                  fname, msgq->name, errno);
    }

    cpr_free(msgq);
    return CPR_SUCCESS;
}















cprRC_t
cprSetMessageQueueThread (cprMsgQueue_t msgQueue, cprThread_t thread)
{
    static const char fname[] = "cprSetMessageQueueThread";
    cpr_msg_queue_t *msgq;

    if ((!msgQueue) || (!thread)) {
        CPR_ERROR("%s: Invalid input\n", fname);
        return CPR_FAILURE;
    }

    msgq = (cpr_msg_queue_t *) msgQueue;
    if (msgq->thread != 0) {
        CPR_ERROR("%s: over-writing previously msgq thread name for %s",
                  fname, msgq->name);
    }

    msgq->thread = cprGetThreadId(thread);
    return CPR_SUCCESS;
}





















void *
cprGetMessage (cprMsgQueue_t msgQueue, boolean waitForever, void **ppUserData)
{
    static const char fname[] = "cprGetMessage";
    struct msgbuffer rcvBuffer = { 0 };
    struct msgbuffer *rcvMsg = &rcvBuffer;
    void *buffer;
    int msgrcvflags;
    cpr_msg_queue_t *msgq;

    
    if (ppUserData) {
        *ppUserData = NULL;
    }

    msgq = (cpr_msg_queue_t *) msgQueue;
    if (msgq == NULL) {
        
        CPR_ERROR("%s: Invalid input\n", fname);
        errno = EINVAL;
        return NULL;
    }

    



    if (waitForever) {
        msgrcvflags = 0;
    } else {
        msgrcvflags = IPC_NOWAIT;
    }

    if (msgrcv(msgq->queueId, rcvMsg,
        sizeof(struct msgbuffer) - offsetof(struct msgbuffer, msgPtr),
        0, msgrcvflags) == -1) {
    	if (!waitForever && errno == ENOMSG) {
    		CPR_INFO("%s: no message on queue %s (non-blocking receive "
                         " operation), returning\n", fname, msgq->name);
    	} else {
    		CPR_ERROR("%s: msgrcv for queue %s failed: %d\n",
                              fname, msgq->name, errno);
        }
        return NULL;
    }
    CPR_INFO("%s: msgrcv success for queue %s \n",fname, msgq->name);

    (void) pthread_mutex_lock(&msgq->mutex);
    
    msgq->currentCount--;
    (void) pthread_mutex_unlock(&msgq->mutex);

    


    if (ppUserData) {
        *ppUserData = rcvMsg->usrPtr;
    }
    buffer = rcvMsg->msgPtr;

    



    if (msgq->extendedQDepth) {
        cprMoveMsgToQueue(msgq);
    }

    return buffer;
}



































cprRC_t
cprSendMessage (cprMsgQueue_t msgQueue, void *msg, void **ppUserData)
{
    static const char fname[] = "cprSendMessage";
    static const char error_str[] = "%s: Msg not sent to %s queue: %s\n";
    cpr_msgq_post_result_e rc;
    cpr_msg_queue_t *msgq;
    int16_t attemptsToSend = CPR_ATTEMPTS_TO_SEND;
    uint16_t numAttempts   = 0;

    
    if (msgQueue == NULL) {
        CPR_ERROR(error_str, fname, "undefined", "invalid input");
        errno = EINVAL;
        return CPR_FAILURE;
    }

    msgq = (cpr_msg_queue_t *) msgQueue;

    


    do {
        (void) pthread_mutex_lock(&msgq->mutex);

        



        if (msgq->extendedQDepth) {
            



            if (msgq->extendedQDepth < msgq->maxExtendedQDepth) {
                rc = cprPostExtendedQMsg(msgq, msg, ppUserData);
                
                if (rc == CPR_MSGQ_POST_SUCCESS) {
                    cprPegSendMessageStats(msgq, numAttempts);
                } else {
                    msgq->sendErrors++;
                }
                (void) pthread_mutex_unlock(&msgq->mutex);

                if (rc == CPR_MSGQ_POST_SUCCESS) {
                    return CPR_SUCCESS;
                }
                else
                {
                    CPR_ERROR(error_str, fname, msgq->name, "no memory");
                    return CPR_FAILURE;
                }
            }

            




            (void) pthread_mutex_unlock(&msgq->mutex);

            




            if (pthread_self() == msgq->thread) {
                msgq->selfQErrors++;
                msgq->sendErrors++;
                CPR_ERROR(error_str, fname, msgq->name, "FULL");
                return CPR_FAILURE;
            }
        } else {
            


            rc = cprPostMessage(msgq, msg, ppUserData);

            




            if (rc == CPR_MSGQ_POST_PENDING) {
                



                if (msgq->maxExtendedQDepth) {
                    rc = cprPostExtendedQMsg(msgq, msg, ppUserData);
                }
            }

            (void) pthread_mutex_unlock(&msgq->mutex);

            if (rc == CPR_MSGQ_POST_SUCCESS) {
                cprPegSendMessageStats(msgq, numAttempts);
                return CPR_SUCCESS;
            } else if (rc == CPR_MSGQ_POST_FAILED) {
                CPR_ERROR("%s: Msg not sent to %s queue: %d\n",
                          fname, msgq->name, errno);
                msgq->sendErrors++;
                



                if (pthread_self() == msgq->thread) {
                    msgq->selfQErrors++;
                }

                return CPR_FAILURE;
            }
            




        }

        



        attemptsToSend--;
        if (attemptsToSend > 0) {
            







            cprSleep(CPR_SND_TIMEOUT_WAIT_INTERVAL);
            msgq->reTries++;
            numAttempts++;
        }
    } while (attemptsToSend > 0);

    CPR_ERROR(error_str, fname, msgq->name, "FULL");
    msgq->sendErrors++;
    return CPR_FAILURE;
}




















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
    struct msgbuffer mbuf;

    




    mbuf.mtype = CPR_IPC_MSG;
    mbuf.msgPtr = msg;

    if (ppUserData != NULL) {
        mbuf.usrPtr = *ppUserData;
    } else {
        mbuf.usrPtr = NULL;
    }

    


    if (msgsnd(msgq->queueId, &mbuf,
    		 sizeof(struct msgbuffer) - offsetof(struct msgbuffer, msgPtr),
               IPC_NOWAIT) != -1) {
        msgq->currentCount++;
        return CPR_MSGQ_POST_SUCCESS;
    }

    



    if (errno == EAGAIN) {
        return CPR_MSGQ_POST_PENDING;
    }

    return CPR_MSGQ_POST_FAILED;
}





















static cpr_msgq_post_result_e
cprPostExtendedQMsg (cpr_msg_queue_t *msgq, void *msg, void **ppUserData)
{
    cpr_msgq_node_t *node;

    


    node = cpr_malloc(sizeof(*node));
    if (!node) {
        errno = ENOMEM;
        return CPR_MSGQ_POST_FAILED;
    }

    


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
    msgq->extendedQDepth++;
    msgq->currentCount++;

    return CPR_MSGQ_POST_SUCCESS;
}













static void
cprMoveMsgToQueue (cpr_msg_queue_t *msgq)
{
    static const char *fname = "cprMoveMsgToQueue";
    cpr_msgq_post_result_e rc;
    cpr_msgq_node_t *node;

    (void) pthread_mutex_lock(&msgq->mutex);

    if (!msgq->tail) {
        
        CPR_ERROR("%s: MsgQ (%s) list is corrupt", fname, msgq->name);
        (void) pthread_mutex_unlock(&msgq->mutex);
        return;
    }

    node = msgq->tail;

    rc = cprPostMessage(msgq, node->msg, &node->pUserData);
    if (rc == CPR_MSGQ_POST_SUCCESS) {
        


        msgq->tail = node->prev;
        if (msgq->tail) {
            msgq->tail->next = NULL;
        }
        if (msgq->head == node) {
            msgq->head = NULL;
        }
        msgq->extendedQDepth--;
        



        msgq->currentCount--;
    }

    (void) pthread_mutex_unlock(&msgq->mutex);

    if (rc == CPR_MSGQ_POST_SUCCESS) {
        cpr_free(node);
    } else {
        CPR_ERROR("%s: Failed to repost msg on %s queue: %d\n",
                  fname, msgq->name, errno);
    }
}






















uint16_t cprGetDepth (cprMsgQueue_t msgQueue)
{
        cpr_msg_queue_t *msgq;
        msgq = (cpr_msg_queue_t *) msgQueue;
        return msgq->currentCount;
}

