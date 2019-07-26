





#ifndef mozilla_PreallocatedProcessManager_h
#define mozilla_PreallocatedProcessManager_h

#include "base/basictypes.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {
class ContentParent;
}



















class PreallocatedProcessManager MOZ_FINAL
{
  typedef mozilla::dom::ContentParent ContentParent;

public:
  







  static void AllocateAfterDelay();

  





  static void AllocateOnIdle();

  





  static void AllocateNow();

  










  static already_AddRefed<ContentParent> Take();

private:
  PreallocatedProcessManager();
  DISALLOW_EVIL_CONSTRUCTORS(PreallocatedProcessManager);
};

} 

#endif 
