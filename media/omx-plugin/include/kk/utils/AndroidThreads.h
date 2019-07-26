















#ifndef _LIBS_UTILS_ANDROID_THREADS_H
#define _LIBS_UTILS_ANDROID_THREADS_H

#include <stdint.h>
#include <sys/types.h>

#if defined(HAVE_PTHREADS)
# include <pthread.h>
#endif

#include <utils/ThreadDefs.h>




#ifdef __cplusplus
extern "C" {
#endif


extern int androidCreateThread(android_thread_func_t, void *);


extern int androidCreateThreadEtc(android_thread_func_t entryFunction,
                                  void *userData,
                                  const char* threadName,
                                  int32_t threadPriority,
                                  size_t threadStackSize,
                                  android_thread_id_t *threadId);


extern android_thread_id_t androidGetThreadId();



extern int androidCreateRawThreadEtc(android_thread_func_t entryFunction,
                                     void *userData,
                                     const char* threadName,
                                     int32_t threadPriority,
                                     size_t threadStackSize,
                                     android_thread_id_t *threadId);


extern void androidSetThreadName(const char* name);



typedef int (*android_create_thread_fn)(android_thread_func_t entryFunction,
                                        void *userData,
                                        const char* threadName,
                                        int32_t threadPriority,
                                        size_t threadStackSize,
                                        android_thread_id_t *threadId);

extern void androidSetCreateThreadFunc(android_create_thread_fn func);





extern pid_t androidGetTid();

#ifdef HAVE_ANDROID_OS




extern int androidSetThreadPriority(pid_t tid, int prio);



extern int androidGetThreadPriority(pid_t tid);
#endif

#ifdef __cplusplus
} 
#endif



#ifdef __cplusplus
namespace android {



inline bool createThread(thread_func_t f, void *a) {
    return androidCreateThread(f, a) ? true : false;
}


inline bool createThreadEtc(thread_func_t entryFunction,
                            void *userData,
                            const char* threadName = "android:unnamed_thread",
                            int32_t threadPriority = PRIORITY_DEFAULT,
                            size_t threadStackSize = 0,
                            thread_id_t *threadId = 0)
{
    return androidCreateThreadEtc(entryFunction, userData, threadName,
        threadPriority, threadStackSize, threadId) ? true : false;
}


inline thread_id_t getThreadId() {
    return androidGetThreadId();
}


}; 
#endif  


#endif
