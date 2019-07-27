





#include "ObservedDocShell.h"

#include "TimelineMarker.h"
#include "mozilla/Move.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/ToJSValue.h"

namespace mozilla {

using dom::ProfileTimelineMarker;
using dom::ProfileTimelineLayerRect;

ObservedDocShell::ObservedDocShell(nsDocShell* aDocShell)
  : mDocShell(aDocShell)
{}

void
ObservedDocShell::AddMarker(const char* aName, TracingMetadata aMetaData)
{
  TimelineMarker* marker = new TimelineMarker(mDocShell, aName, aMetaData);
  mTimelineMarkers.AppendElement(marker);
}

void
ObservedDocShell::AddMarker(UniquePtr<TimelineMarker>&& aMarker)
{
  mTimelineMarkers.AppendElement(Move(aMarker));
}

void
ObservedDocShell::ClearMarkers()
{
  mTimelineMarkers.Clear();
}

bool
ObservedDocShell::PopMarkers(JSContext* aCx,
                             JS::MutableHandle<JS::Value> aStore)
{
  nsTArray<ProfileTimelineMarker> webidlStore;
  dom::SequenceRooter<ProfileTimelineMarker> rooter(aCx, &webidlStore);

  
  
  nsTArray<UniquePtr<TimelineMarker>> keptStartMarkers;

  for (uint32_t i = 0; i < mTimelineMarkers.Length(); ++i) {
    UniquePtr<TimelineMarker>& startPayload = mTimelineMarkers[i];

    
    
    if (startPayload->GetMetaData() == TRACING_TIMESTAMP) {
      ProfileTimelineMarker* marker = webidlStore.AppendElement();
      marker->mName = NS_ConvertUTF8toUTF16(startPayload->GetName());
      marker->mStart = startPayload->GetTime();
      marker->mEnd = startPayload->GetTime();
      marker->mStack = startPayload->GetStack();
      startPayload->AddDetails(aCx, *marker);
      continue;
    }

    
    
    if (startPayload->GetMetaData() == TRACING_INTERVAL_START) {
      bool hasSeenEnd = false;

      
      
      
      
      bool startIsPaintType = strcmp(startPayload->GetName(), "Paint") == 0;
      bool hasSeenLayerType = false;

      
      
      dom::Sequence<ProfileTimelineLayerRect> layerRectangles;

      
      
      
      uint32_t markerDepth = 0;

      
      
      
      for (uint32_t j = i + 1; j < mTimelineMarkers.Length(); ++j) {
        UniquePtr<TimelineMarker>& endPayload = mTimelineMarkers[j];
        bool endIsLayerType = strcmp(endPayload->GetName(), "Layer") == 0;

        
        if (startIsPaintType && endIsLayerType) {
          hasSeenLayerType = true;
          endPayload->AddLayerRectangles(layerRectangles);
        }
        if (!startPayload->Equals(*endPayload)) {
          continue;
        }
        if (endPayload->GetMetaData() == TRACING_INTERVAL_START) {
          ++markerDepth;
          continue;
        }
        if (endPayload->GetMetaData() == TRACING_INTERVAL_END) {
          if (markerDepth > 0) {
            --markerDepth;
            continue;
          }
          if (!startIsPaintType || (startIsPaintType && hasSeenLayerType)) {
            ProfileTimelineMarker* marker = webidlStore.AppendElement();
            marker->mName = NS_ConvertUTF8toUTF16(startPayload->GetName());
            marker->mStart = startPayload->GetTime();
            marker->mEnd = endPayload->GetTime();
            marker->mStack = startPayload->GetStack();
            if (hasSeenLayerType) {
              marker->mRectangles.Construct(layerRectangles);
            }
            startPayload->AddDetails(aCx, *marker);
            endPayload->AddDetails(aCx, *marker);
          }
          hasSeenEnd = true;
          break;
        }
      }

      
      if (!hasSeenEnd) {
        keptStartMarkers.AppendElement(Move(mTimelineMarkers[i]));
        mTimelineMarkers.RemoveElementAt(i);
        --i;
      }
    }
  }

  mTimelineMarkers.SwapElements(keptStartMarkers);

  return ToJSValue(aCx, webidlStore, aStore);
}

} 
