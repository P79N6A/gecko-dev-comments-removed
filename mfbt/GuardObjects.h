







#ifndef mozilla_GuardObjects_h
#define mozilla_GuardObjects_h

#include "mozilla/Assertions.h"
#include "mozilla/NullPtr.h"
#include "mozilla/Types.h"

#ifdef __cplusplus

#ifdef DEBUG

namespace mozilla {
namespace detail {

















































class GuardObjectNotifier
{
private:
  bool* mStatementDone;

public:
  GuardObjectNotifier() : mStatementDone(nullptr) { }

  ~GuardObjectNotifier() { *mStatementDone = true; }

  void setStatementDone(bool* aStatementIsDone)
  {
    mStatementDone = aStatementIsDone;
  }
};

class GuardObjectNotificationReceiver
{
private:
  bool mStatementDone;

public:
  GuardObjectNotificationReceiver() : mStatementDone(false) { }

  ~GuardObjectNotificationReceiver() {
    




    MOZ_ASSERT(mStatementDone);
  }

  void init(const GuardObjectNotifier& aConstNotifier)
  {
    



    GuardObjectNotifier& notifier =
      const_cast<GuardObjectNotifier&>(aConstNotifier);
    notifier.setStatementDone(&mStatementDone);
  }
};

} 
} 

#endif 

#ifdef DEBUG
#  define MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER \
     mozilla::detail::GuardObjectNotificationReceiver _mCheckNotUsedAsTemporary;
#  define MOZ_GUARD_OBJECT_NOTIFIER_PARAM \
     , const mozilla::detail::GuardObjectNotifier& _notifier = \
         mozilla::detail::GuardObjectNotifier()
#  define MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM \
     const mozilla::detail::GuardObjectNotifier& _notifier = \
         mozilla::detail::GuardObjectNotifier()
#  define MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL \
     , const mozilla::detail::GuardObjectNotifier& _notifier
#  define MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL \
     const mozilla::detail::GuardObjectNotifier& _notifier
#  define MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT \
     , _notifier
#  define MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_TO_PARENT \
       _notifier
#  define MOZ_GUARD_OBJECT_NOTIFIER_INIT \
     do { _mCheckNotUsedAsTemporary.init(_notifier); } while (0)
#else
#  define MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
#  define MOZ_GUARD_OBJECT_NOTIFIER_PARAM
#  define MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM
#  define MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL
#  define MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_IN_IMPL
#  define MOZ_GUARD_OBJECT_NOTIFIER_ONLY_PARAM_TO_PARENT
#  define MOZ_GUARD_OBJECT_NOTIFIER_PARAM_TO_PARENT
#  define MOZ_GUARD_OBJECT_NOTIFIER_INIT do { } while (0)
#endif

#endif 

#endif 
