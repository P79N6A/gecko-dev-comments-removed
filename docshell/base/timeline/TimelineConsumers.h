





#ifndef mozilla_TimelineConsumers_h_
#define mozilla_TimelineConsumers_h_

#include "mozilla/LinkedList.h"
#include "mozilla/UniquePtr.h"
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
  static void AddConsumer(nsDocShell* aDocShell,
                          UniquePtr<ObservedDocShell>& aObservedPtr);
  static void RemoveConsumer(nsDocShell* aDocShell,
                             UniquePtr<ObservedDocShell>& aObservedPtr);
  static bool IsEmpty();
  static bool GetKnownDocShells(Vector<nsRefPtr<nsDocShell>>& aStore);
};

} 

#endif 
