





#ifndef mozilla_AutoGlobalTimelineMarker_h_
#define mozilla_AutoGlobalTimelineMarker_h_

#include "mozilla/GuardObjects.h"
#include "mozilla/Vector.h"
#include "nsRefPtr.h"

class nsDocShell;

namespace mozilla {
















class MOZ_STACK_CLASS AutoGlobalTimelineMarker
{
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;

  
  bool mOk;

  
  mozilla::Vector<nsRefPtr<nsDocShell>> mDocShells;

  
  const char* mName;

  void PopulateDocShells();

public:
  explicit AutoGlobalTimelineMarker(const char* aName
                                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM);

  ~AutoGlobalTimelineMarker();

  AutoGlobalTimelineMarker(const AutoGlobalTimelineMarker& aOther) = delete;
  void operator=(const AutoGlobalTimelineMarker& aOther) = delete;
};

} 

#endif 
