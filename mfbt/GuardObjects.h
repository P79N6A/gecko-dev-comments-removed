






#ifndef mozilla_GuardObjects_h
#define mozilla_GuardObjects_h

#include "mozilla/Assertions.h"
#include "mozilla/Types.h"

#ifdef __cplusplus

#ifdef DEBUG

namespace mozilla {
namespace detail {

















































class MOZ_EXPORT GuardObjectNotifier
{
  private:
    bool* statementDone;

  public:
    GuardObjectNotifier() : statementDone(NULL) { }

    ~GuardObjectNotifier() {
      *statementDone = true;
    }

    void setStatementDone(bool* statementIsDone) {
      statementDone = statementIsDone;
    }
};

class MOZ_EXPORT GuardObjectNotificationReceiver
{
  private:
    bool statementDone;

  public:
    GuardObjectNotificationReceiver() : statementDone(false) { }

    ~GuardObjectNotificationReceiver() {
      




      MOZ_ASSERT(statementDone);
    }

    void init(const GuardObjectNotifier& constNotifier) {
      



      GuardObjectNotifier& notifier = const_cast<GuardObjectNotifier&>(constNotifier);
      notifier.setStatementDone(&statementDone);
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
