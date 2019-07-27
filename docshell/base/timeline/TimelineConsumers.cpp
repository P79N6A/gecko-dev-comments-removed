





#include "mozilla/TimelineConsumers.h"

namespace mozilla {

unsigned long TimelineConsumers::sActiveConsumers = 0;
LinkedList<ObservedDocShell>* TimelineConsumers::sObservedDocShells = nullptr;

LinkedList<ObservedDocShell>&
TimelineConsumers::GetOrCreateObservedDocShellsList()
{
  if (!sObservedDocShells) {
    sObservedDocShells = new LinkedList<ObservedDocShell>();
  }
  return *sObservedDocShells;
}

void
TimelineConsumers::AddConsumer(nsDocShell* aDocShell,
                               UniquePtr<ObservedDocShell>& aObservedPtr)
{
  MOZ_ASSERT(!aObservedPtr);
  sActiveConsumers++;
  aObservedPtr.reset(new ObservedDocShell(aDocShell));
  GetOrCreateObservedDocShellsList().insertFront(aObservedPtr.get());
}

void
TimelineConsumers::RemoveConsumer(nsDocShell* aDocShell,
                                  UniquePtr<ObservedDocShell>& aObservedPtr)
{
  MOZ_ASSERT(aObservedPtr);
  sActiveConsumers--;
  aObservedPtr.get()->ClearMarkers();
  aObservedPtr.get()->remove();
  aObservedPtr.reset(nullptr);
}

bool
TimelineConsumers::IsEmpty()
{
  return sActiveConsumers == 0;
}

bool
TimelineConsumers::GetKnownDocShells(Vector<nsRefPtr<nsDocShell>>& aStore)
{
  const LinkedList<ObservedDocShell>& docShells = GetOrCreateObservedDocShellsList();

  for (const ObservedDocShell* rds = docShells.getFirst();
       rds != nullptr;
       rds = rds->getNext()) {
    if (!aStore.append(**rds)) {
      return false;
    }
  }

  return true;
}

} 
