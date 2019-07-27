





#ifndef mozilla_TimelineConsumers_h_
#define mozilla_TimelineConsumers_h_

#include "mozilla/UniquePtr.h"
#include "mozilla/LinkedList.h"
#include "mozilla/Vector.h"
#include "timeline/ObservedDocShell.h"

class nsDocShell;

namespace mozilla {

class TimelineConsumers
{
private:
  
  static unsigned long sActiveConsumers;
  static LinkedList<ObservedDocShell>* sObservedDocShells;
  static LinkedList<ObservedDocShell>& GetOrCreateObservedDocShellsList();

public:
  static void AddConsumer(nsDocShell* aDocShell);
  static void RemoveConsumer(nsDocShell* aDocShell);
  static bool IsEmpty();
  static bool GetKnownDocShells(Vector<nsRefPtr<nsDocShell>>& aStore);

  
  
  
  static void AddMarkerForDocShell(nsDocShell* aDocShell,
                                   UniquePtr<TimelineMarker>&& aMarker);
  static void AddMarkerForDocShell(nsDocShell* aDocShell,
                                   const char* aName, TracingMetadata aMetaData);
  static void AddMarkerToDocShellsList(Vector<nsRefPtr<nsDocShell>>& aDocShells,
                                       const char* aName, TracingMetadata aMetaData);
  static void AddMarkerToAllObservedDocShells(const char* aName, TracingMetadata aMetaData);
};

} 

#endif 
