





#ifndef AutoTimelineMarker_h__
#define AutoTimelineMarker_h__

#include "mozilla/GuardObjects.h"
#include "mozilla/Vector.h"

#include "nsRefPtr.h"

class nsIDocShell;
class nsDocShell;

namespace mozilla {














class MOZ_STACK_CLASS AutoTimelineMarker
{
  MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER;

  nsRefPtr<nsDocShell> mDocShell;
  const char* mName;

  bool DocShellIsRecording(nsDocShell& aDocShell);

public:
  explicit AutoTimelineMarker(nsIDocShell* aDocShell, const char* aName
                              MOZ_GUARD_OBJECT_NOTIFIER_PARAM);
  ~AutoTimelineMarker();

  AutoTimelineMarker(const AutoTimelineMarker& aOther) = delete;
  void operator=(const AutoTimelineMarker& aOther) = delete;
};
















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
