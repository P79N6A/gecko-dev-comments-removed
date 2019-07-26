



#ifndef _CPR_WIN_IPC_H_
#define _CPR_WIN_IPC_H_

#include "cpr_threads.h"


#define CPR_USE_SET_MESSAGE_QUEUE_THREAD


#define CPR_MAX_MSG_SIZE  8192


#define PHONE_IPC_MSG 0xF005


struct msgbuffer {
    int32_t mtype; 
    void *msgPtr;  
    void *usrPtr;  
};

#endif
