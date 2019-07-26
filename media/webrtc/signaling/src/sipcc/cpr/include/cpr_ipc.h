



#ifndef _CPR_IPC_H_
#define _CPR_IPC_H_

#include "cpr_types.h"
#include "cpr_threads.h"

#ifndef SIP_OS_WINDOWS
#include <pthread.h>
#endif 

__BEGIN_DECLS




typedef void* cprMsgQueue_t;






#define WAIT_FOREVER -1


#define CPR_USE_SET_MESSAGE_QUEUE_THREAD


#define CPR_MAX_MSG_SIZE  8192


#ifdef SIP_OS_WINDOWS
#define PHONE_IPC_MSG 0xF005


struct msgbuffer {
    int32_t mtype; 
    void *msgPtr;  
    void *usrPtr;  
};

#else
#define PHONE_IPC_MSG 1




extern pthread_mutex_t msgQueueListMutex;

#endif 




























cprMsgQueue_t
cprCreateMessageQueue(const char *name, uint16_t depth);


#ifdef CPR_USE_SET_MESSAGE_QUEUE_THREAD













cprRC_t
cprSetMessageQueueThread(cprMsgQueue_t msgQueue, cprThread_t thread);
#endif






















void *
cprGetMessage(cprMsgQueue_t msgQueue,
              boolean waitForever,
              void** usrPtr);


































cprRC_t
cprSendMessage(cprMsgQueue_t msgQueue,
               void* msg,
               void** usrPtr);






uint16_t cprGetDepth(cprMsgQueue_t msgQueue);

__END_DECLS

#endif

