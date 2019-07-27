



#include "cpr.h"
#include "cpr_stdlib.h"
#include "cpr_stdio.h"
#include "plat_api.h"
#include "cpr_string.h"

#include <errno.h>

#define OS_MSGTQL 31 /* need to check number for MV linux and put here */






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
    int32_t queueId;
    uint16_t currentCount;
    uint32_t totalCount;
    uint32_t sendErrors;
    uint32_t reTries;
    uint32_t highAttempts;
    uint32_t selfQErrors;
    uint16_t extendedQDepth;
    uint16_t maxExtendedQDepth;
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

    return CPR_MSGQ_POST_SUCCESS;

}



















void *
cprGetMessage (cprMsgQueue_t msgQueue, boolean waitForever, void **ppUserData)
{
    return NULL;
}















uint16_t cprGetDepth (cprMsgQueue_t msgQueue)
{
    return 0;
}

