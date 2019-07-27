





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
  for (uint32_t i = 0; i < Length(); ++i) {
    (mChain[i]->*aMethod)();
  }
}

bool
OverscrollHandoffChain::AnyApzc(APZCPredicate aPredicate) const
{
  MOZ_ASSERT(Length() > 0);
  for (uint32_t i = 0; i < Length(); ++i) {
    if ((mChain[i]->*aPredicate)()) {
      return true;
    }
  }
  return false;
}

void
OverscrollHandoffChain::FlushRepaints() const
{
  ForEachApzc(&AsyncPanZoomController::FlushRepaintForOverscrollHandoff);
}

void
OverscrollHandoffChain::CancelAnimations(CancelAnimationFlags aFlags) const
{
  MOZ_ASSERT(Length() > 0);
  for (uint32_t i = 0; i < Length(); ++i) {
    mChain[i]->CancelAnimation(aFlags);
  }
}

void
OverscrollHandoffChain::ClearOverscroll() const
{
  ForEachApzc(&AsyncPanZoomController::ClearOverscroll);
}

void
OverscrollHandoffChain::SnapBackOverscrolledApzc(const AsyncPanZoomController* aStart) const
{
  uint32_t i = IndexOf(aStart);
  for (; i < Length(); ++i) {
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

bool
OverscrollHandoffChain::CanScrollInDirection(const AsyncPanZoomController* aApzc,
                                             Layer::ScrollDirection aDirection) const
{
  
  uint32_t i = IndexOf(aApzc);

  
  
  for (uint32_t j = i; j < Length(); ++j) {
    if (mChain[j]->CanScroll(aDirection)) {
      return true;
    }
  }

  return false;
}

bool
OverscrollHandoffChain::HasOverscrolledApzc() const
{
  return AnyApzc(&AsyncPanZoomController::IsOverscrolled);
}

bool
OverscrollHandoffChain::HasFastMovingApzc() const
{
  return AnyApzc(&AsyncPanZoomController::IsMovingFast);
}

nsRefPtr<AsyncPanZoomController>
OverscrollHandoffChain::FindFirstScrollable(const ScrollWheelInput& aInput) const
{
  for (size_t i = 0; i < Length(); i++) {
    if (mChain[i]->CanScroll(aInput)) {
      return mChain[i];
    }
  }
  return nullptr;
}

} 
} 
