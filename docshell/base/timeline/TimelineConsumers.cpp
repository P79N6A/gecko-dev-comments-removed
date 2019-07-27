





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
TimelineConsumers::AddConsumer(nsDocShell* aDocShell)
{
  UniquePtr<ObservedDocShell>& observed = aDocShell->mObserved;

  MOZ_ASSERT(!observed);
  sActiveConsumers++;
  observed.reset(new ObservedDocShell(aDocShell));
  GetOrCreateObservedDocShellsList().insertFront(observed.get());
}

void
TimelineConsumers::RemoveConsumer(nsDocShell* aDocShell)
{
  UniquePtr<ObservedDocShell>& observed = aDocShell->mObserved;

  MOZ_ASSERT(observed);
  sActiveConsumers--;
  observed.get()->ClearMarkers();
  observed.get()->remove();
  observed.reset(nullptr);
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

void
TimelineConsumers::AddMarkerForDocShell(nsDocShell* aDocShell,
                                        UniquePtr<TimelineMarker>&& aMarker)
{
  if (aDocShell->IsObserved()) {
    aDocShell->mObserved->AddMarker(Move(aMarker));
  }
}

void
TimelineConsumers::AddMarkerForDocShell(nsDocShell* aDocShell,
                                        const char* aName, TracingMetadata aMetaData)
{
  if (aDocShell->IsObserved()) {
    aDocShell->mObserved->AddMarker(aName, aMetaData);
  }
}

void
TimelineConsumers::AddMarkerToDocShellsList(Vector<nsRefPtr<nsDocShell>>& aDocShells,
                                            const char* aName, TracingMetadata aMetaData)
{
  for (Vector<nsRefPtr<nsDocShell>>::Range range = aDocShells.all();
       !range.empty();
       range.popFront()) {
    AddMarkerForDocShell(range.front(), aName, aMetaData);
  }
}

void
TimelineConsumers::AddMarkerToAllObservedDocShells(const char* aName, TracingMetadata aMetaData)
{
  Vector<nsRefPtr<nsDocShell>> docShells;
  if (!GetKnownDocShells(docShells)) {
    
    
    return;
  }

  AddMarkerToDocShellsList(docShells, aName, aMetaData);
}

} 
