



#ifndef BASE_THREADING_THREAD_COLLISION_WARNER_H_
#define BASE_THREADING_THREAD_COLLISION_WARNER_H_

#include <memory>

#include "base/atomicops.h"
#include "base/base_export.h"
#include "base/basictypes.h"
#include "base/compiler_specific.h"
























































































#if !defined(NDEBUG)



#define DFAKE_MUTEX(obj) \
     mutable base::ThreadCollisionWarner obj


#define DFAKE_SCOPED_LOCK(obj) \
     base::ThreadCollisionWarner::ScopedCheck s_check_##obj(&obj)


#define DFAKE_SCOPED_RECURSIVE_LOCK(obj) \
     base::ThreadCollisionWarner::ScopedRecursiveCheck sr_check_##obj(&obj)

#define DFAKE_SCOPED_LOCK_THREAD_LOCKED(obj) \
     base::ThreadCollisionWarner::Check check_##obj(&obj)

#else

#define DFAKE_MUTEX(obj) typedef void InternalFakeMutexType##obj
#define DFAKE_SCOPED_LOCK(obj) ((void)0)
#define DFAKE_SCOPED_RECURSIVE_LOCK(obj) ((void)0)
#define DFAKE_SCOPED_LOCK_THREAD_LOCKED(obj) ((void)0)

#endif

namespace base {





struct BASE_EXPORT AsserterBase {
  virtual ~AsserterBase() {}
  virtual void warn() = 0;
};

struct BASE_EXPORT DCheckAsserter : public AsserterBase {
  ~DCheckAsserter() override {}
  void warn() override;
};

class BASE_EXPORT ThreadCollisionWarner {
 public:
  
  explicit ThreadCollisionWarner(AsserterBase* asserter = new DCheckAsserter())
      : valid_thread_id_(0),
        counter_(0),
        asserter_(asserter) {}

  ~ThreadCollisionWarner() {
    delete asserter_;
  }

  
  
  
  
  
  class BASE_EXPORT Check {
   public:
    explicit Check(ThreadCollisionWarner* warner)
        : warner_(warner) {
      warner_->EnterSelf();
    }

    ~Check() {}

   private:
    ThreadCollisionWarner* warner_;

    DISALLOW_COPY_AND_ASSIGN(Check);
  };

  
  
  class BASE_EXPORT ScopedCheck {
   public:
    explicit ScopedCheck(ThreadCollisionWarner* warner)
        : warner_(warner) {
      warner_->Enter();
    }

    ~ScopedCheck() {
      warner_->Leave();
    }

   private:
    ThreadCollisionWarner* warner_;

    DISALLOW_COPY_AND_ASSIGN(ScopedCheck);
  };

  
  
  class BASE_EXPORT ScopedRecursiveCheck {
   public:
    explicit ScopedRecursiveCheck(ThreadCollisionWarner* warner)
        : warner_(warner) {
      warner_->EnterSelf();
    }

    ~ScopedRecursiveCheck() {
      warner_->Leave();
    }

   private:
    ThreadCollisionWarner* warner_;

    DISALLOW_COPY_AND_ASSIGN(ScopedRecursiveCheck);
  };

 private:
  
  
  
  void EnterSelf();

  
  void Enter();

  
  
  void Leave();

  
  
  volatile subtle::Atomic32 valid_thread_id_;

  
  
  volatile subtle::Atomic32 counter_;

  
  
  AsserterBase* asserter_;

  DISALLOW_COPY_AND_ASSIGN(ThreadCollisionWarner);
};

}  

#endif  
