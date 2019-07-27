





#ifndef mozilla_AutoTimelineMarker_h_
#define mozilla_AutoTimelineMarker_h_

#include "mozilla/GuardObjects.h"
#include "nsRefPtr.h"

class nsIDocShell;
class nsDocShell;

namespace mozilla {














class MOZ_STACK_CLASS AutoTimelineMarker
{
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;

  
  const char* mName;

  
  nsRefPtr<nsDocShell> mDocShell;

public:
  explicit AutoTimelineMarker(nsIDocShell* aDocShell, const char* aName
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
  ~AutoTimelineMarker();

  AutoTimelineMarker(const AutoTimelineMarker& aOther) = delete;
  void operator=(const AutoTimelineMarker& aOther) = delete;
};

} 

#endif 
