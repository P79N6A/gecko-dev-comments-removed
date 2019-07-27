





#include "OverscrollHandoffState.h"

#include <algorithm>              
#include "mozilla/Assertions.h"
#include "AsyncPanZoomController.h"

namespace mozilla {
namespace layers {

OverscrollHandoffChain::~OverscrollHandoffChain() {}

void
OverscrollHandoffChain::Add(AsyncPanZoomController* aApzc)
{
  mChain.push_back(aApzc);
}

struct CompareByScrollPriority
{
  bool operator()(const nsRefPtr<AsyncPanZoomController>& a,
                  const nsRefPtr<AsyncPanZoomController>& b) const
  {
    return a->HasScrollgrab() && !b->HasScrollgrab();
  }
};

void
OverscrollHandoffChain::SortByScrollPriority()
{
  
  
  
  
  
  std::stable_sort(mChain.begin(), mChain.end(), CompareByScrollPriority());
}

const nsRefPtr<AsyncPanZoomController>&
OverscrollHandoffChain::GetApzcAtIndex(uint32_t aIndex) const
{
  MOZ_ASSERT(aIndex < Length());
  return mChain[aIndex];
}

uint32_t
OverscrollHandoffChain::IndexOf(const AsyncPanZoomController* aApzc) const
{
  uint32_t i;
  for (i = 0; i < Length(); ++i) {
    if (mChain[i] == aApzc) {
      break;
    }
  }
  return i;
}

void
OverscrollHandoffChain::ForEachApzc(APZCMethod aMethod) const
{
  MOZ_ASSERT(Length() > 0);
  for (uint32_t i = 0; i < Length(); ++i) {
    (mChain[i]->*aMethod)();
  }
}

void
OverscrollHandoffChain::FlushRepaints() const
{
  ForEachApzc(&AsyncPanZoomController::FlushRepaintForOverscrollHandoff);
}

void
OverscrollHandoffChain::CancelAnimations() const
{
  ForEachApzc(&AsyncPanZoomController::CancelAnimation);
}

void
OverscrollHandoffChain::ClearOverscroll() const
{
  ForEachApzc(&AsyncPanZoomController::ClearOverscroll);
}

void
OverscrollHandoffChain::SnapBackOverscrolledApzc() const
{
  uint32_t i = 0;
  for (i = 0; i < Length(); ++i) {
    AsyncPanZoomController* apzc = mChain[i];
    if (!apzc->IsDestroyed() && apzc->SnapBackIfOverscrolled()) {
      
      break;
    }
  }

  
#ifdef DEBUG
  ++i;
  for (; i < Length(); ++i) {
    AsyncPanZoomController* apzc = mChain[i];
    if (!apzc->IsDestroyed()) {
      MOZ_ASSERT(!apzc->IsOverscrolled());
    }
  }
#endif
}

bool
OverscrollHandoffChain::CanBePanned(const AsyncPanZoomController* aApzc) const
{
  
  uint32_t i = IndexOf(aApzc);

  
  
  for (uint32_t j = i; j < Length(); ++j) {
    if (mChain[j]->IsPannable()) {
      return true;
    }
  }

  return false;
}

} 
} 
