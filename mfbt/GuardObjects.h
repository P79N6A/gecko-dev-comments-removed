







































#ifndef mozilla_GuardObjects_h
#define mozilla_GuardObjects_h

#include "mozilla/Types.h"
#include "mozilla/Util.h"

namespace mozilla {
  










































#ifdef DEBUG
  class GuardObjectNotifier
  {
  private:
    bool* mStatementDone;
  public:
    GuardObjectNotifier() : mStatementDone(NULL) {}

    ~GuardObjectNotifier() {
      *mStatementDone = true;
    }

    void SetStatementDone(bool *aStatementDone) {
      mStatementDone = aStatementDone;
    }
  };

  class GuardObjectNotificationReceiver
  {
  private:
    bool mStatementDone;
  public:
    GuardObjectNotificationReceiver() : mStatementDone(false) {}

    ~GuardObjectNotificationReceiver() {
      





      MOZ_ASSERT(mStatementDone);
    }

    void Init(const GuardObjectNotifier &aNotifier) {
      



      const_cast<GuardObjectNotifier&>(aNotifier).
          SetStatementDone(&mStatementDone);
    }
  };

  #define MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER \
      mozilla::GuardObjectNotificationReceiver _mCheckNotUsedAsTemporary;
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM \
      , const mozilla::GuardObjectNotifier& _notifier = \
                mozilla::GuardObjectNotifier()
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_ONLY_PARAM \
      const mozilla::GuardObjectNotifier& _notifier = \
              mozilla::GuardObjectNotifier()
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL \
      , const mozilla::GuardObjectNotifier& _notifier
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT \
      , _notifier
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_INIT \
      PR_BEGIN_MACRO _mCheckNotUsedAsTemporary.Init(_notifier); PR_END_MACRO

#else 

  #define MOZILLA_DECL_USE_GUARD_OBJECT_NOTIFIER
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_ONLY_PARAM
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT
  #define MOZILLA_GUARD_OBJECT_NOTIFIER_INIT PR_BEGIN_MACRO PR_END_MACRO

#endif 

} 

#endif 
