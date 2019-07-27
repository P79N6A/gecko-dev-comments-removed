





#ifndef mozilla_ProcessPriorityManager_h_
#define mozilla_ProcessPriorityManager_h_

#include "mozilla/HalTypes.h"

namespace mozilla {
namespace dom {
class ContentParent;
} 














class ProcessPriorityManager final
{
public:
  







  static void Init();

  















  static void SetProcessPriority(dom::ContentParent* aContentParent,
                                 hal::ProcessPriority aPriority);

  






  static bool CurrentProcessIsForeground();

  



  static bool AnyProcessHasHighPriority();

private:
  ProcessPriorityManager();
  DISALLOW_EVIL_CONSTRUCTORS(ProcessPriorityManager);
};

} 

#endif
