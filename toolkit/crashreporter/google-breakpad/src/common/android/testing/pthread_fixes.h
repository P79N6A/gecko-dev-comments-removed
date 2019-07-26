
































#ifndef GOOGLE_BREAKPAD_COMMON_ANDROID_TESTING_PTHREAD_FIXES_H
#define GOOGLE_BREAKPAD_COMMON_ANDROID_TESTING_PTHREAD_FIXES_H

#include <pthread.h>

namespace {


#ifndef PTHREAD_BARRIER_SERIAL_THREAD


#define PTHREAD_BARRIER_SERIAL_THREAD  0x12345

typedef struct {
  pthread_mutex_t  mutex;
  pthread_cond_t   cond;
  unsigned         count;
} pthread_barrier_t;

int pthread_barrier_init(pthread_barrier_t* barrier,
                         const void* ,
                         unsigned count) {
  barrier->count = count;
  pthread_mutex_init(&barrier->mutex, NULL);
  pthread_cond_init(&barrier->cond, NULL);
  return 0;
}

int pthread_barrier_wait(pthread_barrier_t* barrier) {
  
  pthread_mutex_lock(&barrier->mutex);
  
  
  if (--barrier->count == 0) {
    
    pthread_cond_broadcast(&barrier->cond);
    pthread_mutex_unlock(&barrier->mutex);
    return PTHREAD_BARRIER_SERIAL_THREAD;
  }
  
  
  do {
    pthread_cond_wait(&barrier->cond, &barrier->mutex);
  } while (barrier->count > 0);

  pthread_mutex_unlock(&barrier->mutex);
  return 0;
}

int pthread_barrier_destroy(pthread_barrier_t *barrier) {
  barrier->count = 0;
  pthread_cond_destroy(&barrier->cond);
  pthread_mutex_destroy(&barrier->mutex);
}

#endif  

int pthread_yield(void) {
  sched_yield();
  return 0;
}

}  

#endif  
