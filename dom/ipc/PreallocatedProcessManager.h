





#ifndef mozilla_PreallocatedProcessManager_h
#define mozilla_PreallocatedProcessManager_h

#include "base/basictypes.h"
#include "nsCOMPtr.h"
#include "nsIObserver.h"

class nsIRunnable;

namespace mozilla {
namespace dom {
class ContentParent;
}



















class PreallocatedProcessManager final
{
  typedef mozilla::dom::ContentParent ContentParent;

public:
  







  static void AllocateAfterDelay();

  





  static void AllocateOnIdle();

  





  static void AllocateNow();

  










  static already_AddRefed<ContentParent> Take();

#ifdef MOZ_NUWA_PROCESS
  static void PublishSpareProcess(ContentParent* aContent);
  static void MaybeForgetSpare(ContentParent* aContent);
  static bool IsNuwaReady();
  static void OnNuwaReady();
  static bool PreallocatedProcessReady();
#endif

private:
  PreallocatedProcessManager();
  DISALLOW_EVIL_CONSTRUCTORS(PreallocatedProcessManager);
};

} 

#endif 
