









#ifndef WEBRTC_BASE_REFERENCECOUNTEDSINGLETONFACTORY_H_
#define WEBRTC_BASE_REFERENCECOUNTEDSINGLETONFACTORY_H_

#include "webrtc/base/common.h"
#include "webrtc/base/criticalsection.h"
#include "webrtc/base/logging.h"
#include "webrtc/base/scoped_ptr.h"

namespace rtc {

template <typename Interface> class rcsf_ptr;









template <typename Interface>
class ReferenceCountedSingletonFactory {
  friend class rcsf_ptr<Interface>;
 public:
  ReferenceCountedSingletonFactory() : ref_count_(0) {}

  virtual ~ReferenceCountedSingletonFactory() {
    ASSERT(ref_count_ == 0);
  }

 protected:
  
  
  
  virtual bool SetupInstance() = 0;
  virtual void CleanupInstance() = 0;

  scoped_ptr<Interface> instance_;

 private:
  Interface* GetInstance() {
    rtc::CritScope cs(&crit_);
    if (ref_count_ == 0) {
      if (!SetupInstance()) {
        LOG(LS_VERBOSE) << "Failed to setup instance";
        return NULL;
      }
      ASSERT(instance_.get() != NULL);
    }
    ++ref_count_;

    LOG(LS_VERBOSE) << "Number of references: " << ref_count_;
    return instance_.get();
  }

  void ReleaseInstance() {
    rtc::CritScope cs(&crit_);
    ASSERT(ref_count_ > 0);
    ASSERT(instance_.get() != NULL);
    --ref_count_;
    LOG(LS_VERBOSE) << "Number of references: " << ref_count_;
    if (ref_count_ == 0) {
      CleanupInstance();
    }
  }

  CriticalSection crit_;
  int ref_count_;

  DISALLOW_COPY_AND_ASSIGN(ReferenceCountedSingletonFactory);
};

template <typename Interface>
class rcsf_ptr {
 public:
  
  
  explicit rcsf_ptr(ReferenceCountedSingletonFactory<Interface>* factory)
      : instance_(NULL),
        factory_(factory) {
  }

  ~rcsf_ptr() {
    release();
  }

  Interface& operator*() {
    EnsureAcquired();
    return *instance_;
  }

  Interface* operator->() {
    EnsureAcquired();
    return instance_;
  }

  
  
  Interface* get() {
    Acquire();
    return instance_;
  }

  
  
  void release() {
    if (instance_) {
      instance_ = NULL;
      factory_->ReleaseInstance();
    }
  }

  
  
  
  bool valid() const {
    return instance_ != NULL;
  }

  
  ReferenceCountedSingletonFactory<Interface>* factory() const {
    return factory_;
  }

 private:
  void EnsureAcquired() {
    Acquire();
    ASSERT(instance_ != NULL);
  }

  void Acquire() {
    
    
    if (!instance_) {
      instance_ = factory_->GetInstance();
    }
  }

  Interface* instance_;
  ReferenceCountedSingletonFactory<Interface>* factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(rcsf_ptr);
};

};  

#endif  
