















#ifndef VP9_DECODER_VP9_THREAD_H_
#define VP9_DECODER_VP9_THREAD_H_

#include "./vpx_config.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#if CONFIG_MULTITHREAD

#if defined(_WIN32)

#include <windows.h>  
typedef HANDLE pthread_t;
typedef CRITICAL_SECTION pthread_mutex_t;
typedef struct {
  HANDLE waiting_sem_;
  HANDLE received_sem_;
  HANDLE signal_event_;
} pthread_cond_t;

#else

#include <pthread.h> 

#endif    
#endif    


typedef enum {
  NOT_OK = 0,   
  OK,           
  WORK          
} VP9WorkerStatus;



typedef int (*VP9WorkerHook)(void*, void*);


typedef struct {
#if CONFIG_MULTITHREAD
  pthread_mutex_t mutex_;
  pthread_cond_t  condition_;
  pthread_t       thread_;
#endif
  VP9WorkerStatus status_;
  VP9WorkerHook hook;     
  void* data1;            
  void* data2;            
  int had_error;          
} VP9Worker;


void vp9_worker_init(VP9Worker* const worker);


int vp9_worker_reset(VP9Worker* const worker);


int vp9_worker_sync(VP9Worker* const worker);



void vp9_worker_launch(VP9Worker* const worker);




void vp9_worker_execute(VP9Worker* const worker);


void vp9_worker_end(VP9Worker* const worker);



#if defined(__cplusplus) || defined(c_plusplus)
}    
#endif

#endif
