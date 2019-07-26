




#ifndef DEBUG
MOZ_STATIC_ASSERT(false, "This should not be compiled in !DEBUG");
#endif 

#include "nsAutoLayoutPhase.h"
#include "nsPresContext.h"
#include "nsContentUtils.h"

nsAutoLayoutPhase::nsAutoLayoutPhase(nsPresContext* aPresContext,
                                     nsLayoutPhase aPhase)
  : mPresContext(aPresContext)
  , mPhase(aPhase)
  , mCount(0)
{
  Enter();
}

nsAutoLayoutPhase::~nsAutoLayoutPhase()
{
  Exit();
  MOZ_ASSERT(mCount == 0, "imbalanced");
}

void
nsAutoLayoutPhase::Enter()
{
  switch (mPhase) {
    case eLayoutPhase_Paint:
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_Paint] == 0,
                 "recurring into paint");
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_Reflow] == 0,
                 "painting in the middle of reflow");
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_FrameC] == 0,
                 "painting in the middle of frame construction");
      break;
    case eLayoutPhase_Reflow:
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_Paint] == 0,
                 "reflowing in the middle of a paint");
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_Reflow] == 0,
                 "recurring into reflow");
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_FrameC] == 0,
                 "reflowing in the middle of frame construction");
      break;
    case eLayoutPhase_FrameC:
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_Paint] == 0,
                 "constructing frames in the middle of a paint");
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_Reflow] == 0,
                 "constructing frames in the middle of reflow");
      MOZ_ASSERT(mPresContext->mLayoutPhaseCount[eLayoutPhase_FrameC] == 0,
                 "recurring into frame construction");
      MOZ_ASSERT(!nsContentUtils::IsSafeToRunScript(),
                 "constructing frames and scripts are not blocked");
      break;
    case eLayoutPhase_COUNT:
      break;
  }

  ++(mPresContext->mLayoutPhaseCount[mPhase]);
  ++mCount;
}

void
nsAutoLayoutPhase::Exit()
{
  MOZ_ASSERT(mCount > 0 && mPresContext->mLayoutPhaseCount[mPhase] > 0,
             "imbalanced");
  --(mPresContext->mLayoutPhaseCount[mPhase]);
  --mCount;
}
