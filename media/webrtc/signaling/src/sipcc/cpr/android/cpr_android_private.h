



#ifndef _CPR_ANDROID_PRIVATE_H_
#define _CPR_ANDROID_PRIVATE_H_

extern pthread_mutexattr_t cprMutexAttributes;

cpr_status_e cprLockInit(void);
cpr_status_e cprTimerInit(void);

#endif
