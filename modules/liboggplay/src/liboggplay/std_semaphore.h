































    

#ifndef _STD_SEMAPHORE_H
#define _STD_SEMAPHORE_H
#if defined(linux)
#include <semaphore.h>
#define SEM_CREATE(p,s) sem_init(&(p), 1, s) 
#define SEM_SIGNAL(p)   sem_post(&(p))
#define SEM_WAIT(p)     sem_wait(&(p))
#define SEM_CLOSE(p)    sem_destroy(&(p))
typedef sem_t           semaphore;
#elif defined(WIN32)
#include <windows.h>
#define SEM_CREATE(p,s) p = CreateSemaphore(NULL, (long)(s), (long)(s), NULL)
#define SEM_SIGNAL(p)   ReleaseSemaphore(p, 1, NULL)
#define SEM_WAIT(p)     WaitForSingleObject(p, INFINITE)
#define SEM_TEST(p,s)   p = WaitForSingleObject(s, 0)
#define SEM_CLOSE(p)    CloseHandle(p)
typedef HANDLE          semaphore;
#elif defined(__APPLE__)
#include <Multiprocessing.h>
#define SEM_CREATE(p,s) MPCreateSemaphore(s, s, &(p))
#define SEM_SIGNAL(p)   MPSignalSemaphore(p)
#define SEM_WAIT(p)     MPWaitOnSemaphore(p, kDurationForever)
#define SEM_CLOSE(p)    MPDeleteSemaphore(p)
typedef MPSemaphoreID   semaphore;
#endif
#endif





































