



#ifndef _CPR_IPC_H_
#define _CPR_IPC_H_

#include "cpr_types.h"

__BEGIN_DECLS




typedef void* cprMsgQueue_t;






#define WAIT_FOREVER -1

#if defined SIP_OS_LINUX
#include "../linux/cpr_linux_ipc.h"
#elif defined SIP_OS_WINDOWS
#include "../win32/cpr_win_ipc.h"
#elif defined SIP_OS_OSX
#include "../darwin/cpr_darwin_ipc.h"
#endif




























cprMsgQueue_t
cprCreateMessageQueue(const char *name, uint16_t depth);
















cprRC_t
cprDestroyMessageQueue(cprMsgQueue_t msgQueue);

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

__END_DECLS

#endif

