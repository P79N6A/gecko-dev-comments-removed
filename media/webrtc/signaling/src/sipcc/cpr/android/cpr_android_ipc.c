



#include "cpr.h"
#include "cpr_stdlib.h"
#include <cpr_stdio.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <plat_api.h>
#include "cpr_string.h"





#define STATIC static

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




static const char unnamed_string[] = "unnamed";












#define CPR_MAX_MSG_Q_DEPTH 256











#define CPR_SND_TIMEOUT_WAIT_INTERVAL 20












#define CPR_ATTEMPTS_TO_SEND 25















static cpr_msgq_post_result_e
cprPostMessage(cpr_msg_queue_t *msgq, void *msg, void **ppUserData);
static void
cprPegSendMessageStats(cpr_msg_queue_t *msgq, uint16_t numAttempts);




















cprMsgQueue_t
cprCreateMessageQueue (const char *name, uint16_t depth)
{
    static const char fname[] = "cprCreateMessageQueue";
    cpr_msg_queue_t *msgq;
    static int key_id = 100; 

    msgq = cpr_calloc(1, sizeof(cpr_msg_queue_t));
    if (msgq == NULL) {
        printf("%s: Malloc failed: %s\n", fname,
                  name ? name : unnamed_string);
        errno = ENOMEM;
        return NULL;
    }

    msgq->name = name ? name : unnamed_string;
	msgq->queueId = key_id++;

	pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
	msgq->cond = _cond;
	pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
	msgq->mutex = _lock;

    


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

    void *buffer = 0;
    cpr_msg_queue_t *msgq;
    cpr_msgq_node_t *node;
    struct timespec timeout;
    struct timeval tv;
    struct timezone *tz;

    
    if (ppUserData) {
        *ppUserData = NULL;
    }

    msgq = (cpr_msg_queue_t *) msgQueue;
    if (msgq == NULL) {
        
        CPR_ERROR("%s: Invalid input\n", fname);
        errno = EINVAL;
        return NULL;
    }

    




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

		


		rc = cprPostMessage(msgq, msg, ppUserData);

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
















uint16_t cprGetDepth (cprMsgQueue_t msgQueue)
{
        cpr_msg_queue_t *msgq;
        msgq = (cpr_msg_queue_t *) msgQueue;
        return msgq->currentCount;
}

