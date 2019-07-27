



#ifndef _CPR_IPC_H_
#define _CPR_IPC_H_

#include "cpr_types.h"
#include "cpr_threads.h"

__BEGIN_DECLS




typedef void* cprMsgQueue_t;






#define WAIT_FOREVER -1


#define CPR_USE_SET_MESSAGE_QUEUE_THREAD


#define CPR_MAX_MSG_SIZE  8192























void *
cprGetMessage(cprMsgQueue_t msgQueue,
              boolean waitForever,
              void** usrPtr);






uint16_t cprGetDepth(cprMsgQueue_t msgQueue);

__END_DECLS

#endif

