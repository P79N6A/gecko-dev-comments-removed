



#ifndef _CPR_CNU_IPC_H_
#define _CPR_CNU_IPC_H_

#include "cpr_threads.h"
#include <pthread.h>


#define CPR_USE_SET_MESSAGE_QUEUE_THREAD


#define CPR_MAX_MSG_SIZE  8192


#define CPR_IPC_MSG 1



struct msgbuffer {
    long    mtype;    
    void   *msgPtr;   
    void   *usrPtr;   
};


typedef struct {
    char name[16];
    uint16_t currentCount;
    uint32_t totalCount;
    uint32_t rcvTimeouts;
    uint32_t sendErrors;
    uint32_t reTries;
    uint32_t highAttempts;
    uint32_t selfQErrors;
    uint16_t extendedDepth;
} cprMsgQueueStats_t;





extern pthread_mutex_t msgQueueListMutex;







uint16_t cprGetDepth(cprMsgQueue_t msgQueue);

#endif
