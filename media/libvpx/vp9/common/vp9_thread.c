














#include <assert.h>
#include <string.h>   
#include "./vp9_thread.h"
#include "vpx_mem/vpx_mem.h"

#if CONFIG_MULTITHREAD

struct VP9WorkerImpl {
  pthread_mutex_t mutex_;
  pthread_cond_t  condition_;
  pthread_t       thread_;
};



static void execute(VP9Worker *const worker);  

static THREADFN thread_loop(void *ptr) {
  VP9Worker *const worker = (VP9Worker*)ptr;
  int done = 0;
  while (!done) {
    pthread_mutex_lock(&worker->impl_->mutex_);
    while (worker->status_ == OK) {   
      pthread_cond_wait(&worker->impl_->condition_, &worker->impl_->mutex_);
    }
    if (worker->status_ == WORK) {
      execute(worker);
      worker->status_ = OK;
    } else if (worker->status_ == NOT_OK) {   
      done = 1;
    }
    
    pthread_cond_signal(&worker->impl_->condition_);
    pthread_mutex_unlock(&worker->impl_->mutex_);
  }
  return THREAD_RETURN(NULL);    
}


static void change_state(VP9Worker *const worker,
                         VP9WorkerStatus new_status) {
  
  
  
  if (worker->impl_ == NULL) return;

  pthread_mutex_lock(&worker->impl_->mutex_);
  if (worker->status_ >= OK) {
    
    while (worker->status_ != OK) {
      pthread_cond_wait(&worker->impl_->condition_, &worker->impl_->mutex_);
    }
    
    if (new_status != OK) {
      worker->status_ = new_status;
      pthread_cond_signal(&worker->impl_->condition_);
    }
  }
  pthread_mutex_unlock(&worker->impl_->mutex_);
}

#endif  



static void init(VP9Worker *const worker) {
  memset(worker, 0, sizeof(*worker));
  worker->status_ = NOT_OK;
}

static int sync(VP9Worker *const worker) {
#if CONFIG_MULTITHREAD
  change_state(worker, OK);
#endif
  assert(worker->status_ <= OK);
  return !worker->had_error;
}

static int reset(VP9Worker *const worker) {
  int ok = 1;
  worker->had_error = 0;
  if (worker->status_ < OK) {
#if CONFIG_MULTITHREAD
    worker->impl_ = (VP9WorkerImpl*)vpx_calloc(1, sizeof(*worker->impl_));
    if (worker->impl_ == NULL) {
      return 0;
    }
    if (pthread_mutex_init(&worker->impl_->mutex_, NULL)) {
      goto Error;
    }
    if (pthread_cond_init(&worker->impl_->condition_, NULL)) {
      pthread_mutex_destroy(&worker->impl_->mutex_);
      goto Error;
    }
    pthread_mutex_lock(&worker->impl_->mutex_);
    ok = !pthread_create(&worker->impl_->thread_, NULL, thread_loop, worker);
    if (ok) worker->status_ = OK;
    pthread_mutex_unlock(&worker->impl_->mutex_);
    if (!ok) {
      pthread_mutex_destroy(&worker->impl_->mutex_);
      pthread_cond_destroy(&worker->impl_->condition_);
 Error:
      vpx_free(worker->impl_);
      worker->impl_ = NULL;
      return 0;
    }
#else
    worker->status_ = OK;
#endif
  } else if (worker->status_ > OK) {
    ok = sync(worker);
  }
  assert(!ok || (worker->status_ == OK));
  return ok;
}

static void execute(VP9Worker *const worker) {
  if (worker->hook != NULL) {
    worker->had_error |= !worker->hook(worker->data1, worker->data2);
  }
}

static void launch(VP9Worker *const worker) {
#if CONFIG_MULTITHREAD
  change_state(worker, WORK);
#else
  execute(worker);
#endif
}

static void end(VP9Worker *const worker) {
#if CONFIG_MULTITHREAD
  if (worker->impl_ != NULL) {
    change_state(worker, NOT_OK);
    pthread_join(worker->impl_->thread_, NULL);
    pthread_mutex_destroy(&worker->impl_->mutex_);
    pthread_cond_destroy(&worker->impl_->condition_);
    vpx_free(worker->impl_);
    worker->impl_ = NULL;
  }
#else
  worker->status_ = NOT_OK;
  assert(worker->impl_ == NULL);
#endif
  assert(worker->status_ == NOT_OK);
}



static VP9WorkerInterface g_worker_interface = {
  init, reset, sync, launch, execute, end
};

int vp9_set_worker_interface(const VP9WorkerInterface* const winterface) {
  if (winterface == NULL ||
      winterface->init == NULL || winterface->reset == NULL ||
      winterface->sync == NULL || winterface->launch == NULL ||
      winterface->execute == NULL || winterface->end == NULL) {
    return 0;
  }
  g_worker_interface = *winterface;
  return 1;
}

const VP9WorkerInterface *vp9_get_worker_interface(void) {
  return &g_worker_interface;
}


