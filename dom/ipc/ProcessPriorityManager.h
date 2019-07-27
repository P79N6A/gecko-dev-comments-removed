





#ifndef mozilla_ProcessPriorityManager_h_
#define mozilla_ProcessPriorityManager_h_

#include "mozilla/HalTypes.h"

namespace mozilla {
namespace dom {
class ContentParent;
}














class ProcessPriorityManager MOZ_FINAL
{
public:
  







  static void Init();

  















  static void SetProcessPriority(dom::ContentParent* aContentParent,
                                 hal::ProcessPriority aPriority);

  






  static bool CurrentProcessIsForeground();

  



  static bool AnyProcessHasHighPriority();

  



  static void RemoveFromBackgroundLRUPool(dom::ContentParent* aContentParent);

  



  static void AddIntoBackgroundLRUPool(dom::ContentParent* aContentParent);

private:
  ProcessPriorityManager();
  DISALLOW_EVIL_CONSTRUCTORS(ProcessPriorityManager);
};

} 

#endif
