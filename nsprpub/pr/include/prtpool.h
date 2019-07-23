




































#ifndef prtpool_h___
#define prtpool_h___

#include "prtypes.h"
#include "prthread.h"
#include "prio.h"
#include "prerror.h"







PR_BEGIN_EXTERN_C

typedef struct PRJobIoDesc {
    PRFileDesc *socket;
    PRErrorCode error;
    PRIntervalTime timeout;
} PRJobIoDesc;

typedef struct PRThreadPool PRThreadPool;
typedef struct PRJob PRJob;
typedef void (PR_CALLBACK *PRJobFn) (void *arg);


NSPR_API(PRThreadPool *)
PR_CreateThreadPool(PRInt32 initial_threads, PRInt32 max_threads,
                          PRUint32 stacksize);


NSPR_API(PRJob *)
PR_QueueJob(PRThreadPool *tpool, PRJobFn fn, void *arg, PRBool joinable);


NSPR_API(PRJob *)
PR_QueueJob_Read(PRThreadPool *tpool, PRJobIoDesc *iod,
							PRJobFn fn, void * arg, PRBool joinable);


NSPR_API(PRJob *)
PR_QueueJob_Write(PRThreadPool *tpool, PRJobIoDesc *iod,
								PRJobFn fn, void * arg, PRBool joinable);


NSPR_API(PRJob *)
PR_QueueJob_Accept(PRThreadPool *tpool, PRJobIoDesc *iod,
									PRJobFn fn, void * arg, PRBool joinable);


NSPR_API(PRJob *)
PR_QueueJob_Connect(PRThreadPool *tpool, PRJobIoDesc *iod,
			const PRNetAddr *addr, PRJobFn fn, void * arg, PRBool joinable);


NSPR_API(PRJob *)
PR_QueueJob_Timer(PRThreadPool *tpool, PRIntervalTime timeout,
								PRJobFn fn, void * arg, PRBool joinable);

NSPR_API(PRStatus)
PR_CancelJob(PRJob *job);


NSPR_API(PRStatus)
PR_JoinJob(PRJob *job);


NSPR_API(PRStatus)
PR_ShutdownThreadPool(PRThreadPool *tpool);


NSPR_API(PRStatus)
PR_JoinThreadPool(PRThreadPool *tpool);

PR_END_EXTERN_C

#endif 
