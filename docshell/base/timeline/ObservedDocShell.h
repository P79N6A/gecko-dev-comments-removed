





#ifndef ObservedDocShell_h_
#define ObservedDocShell_h_

#include "GeckoProfiler.h"
#include "nsTArray.h"
#include "nsRefPtr.h"

class nsDocShell;
class TimelineMarker;

namespace mozilla {





class ObservedDocShell : public LinkedListElement<ObservedDocShell>
{
private:
  nsRefPtr<nsDocShell> mDocShell;

public:
  
  
  nsTArray<UniquePtr<TimelineMarker>> mTimelineMarkers;

  explicit ObservedDocShell(nsDocShell* aDocShell);
  nsDocShell* operator*() const { return mDocShell.get(); }

  void AddMarker(const char* aName, TracingMetadata aMetaData);
  void AddMarker(UniquePtr<TimelineMarker>&& aMarker);
  void ClearMarkers();
};

} 

#endif 
