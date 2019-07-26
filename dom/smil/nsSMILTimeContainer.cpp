




#include "nsSMILTimeContainer.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimedElement.h"
#include <algorithm>

nsSMILTimeContainer::nsSMILTimeContainer()
:
  mParent(nullptr),
  mCurrentTime(0L),
  mParentOffset(0L),
  mPauseStart(0L),
  mNeedsPauseSample(false),
  mNeedsRewind(false),
  mIsSeeking(false),
  mPauseState(PAUSE_BEGIN)
{
}

nsSMILTimeContainer::~nsSMILTimeContainer()
{
  if (mParent) {
    mParent->RemoveChild(*this);
  }
}

nsSMILTimeValue
nsSMILTimeContainer::ContainerToParentTime(nsSMILTime aContainerTime) const
{
  
  if (IsPaused() && aContainerTime > mCurrentTime)
    return nsSMILTimeValue::Indefinite();

  return nsSMILTimeValue(aContainerTime + mParentOffset);
}

nsSMILTimeValue
nsSMILTimeContainer::ParentToContainerTime(nsSMILTime aParentTime) const
{
  
  if (IsPaused() && aParentTime > mPauseStart)
    return nsSMILTimeValue::Indefinite();

  return nsSMILTimeValue(aParentTime - mParentOffset);
}

void
nsSMILTimeContainer::Begin()
{
  Resume(PAUSE_BEGIN);
  if (mPauseState) {
    mNeedsPauseSample = true;
  }

  
  
  
  
  
  

  UpdateCurrentTime();
}

void
nsSMILTimeContainer::Pause(uint32_t aType)
{
  bool didStartPause = false;

  if (!mPauseState && aType) {
    mPauseStart = GetParentTime();
    mNeedsPauseSample = true;
    didStartPause = true;
  }

  mPauseState |= aType;

  if (didStartPause) {
    NotifyTimeChange();
  }
}

void
nsSMILTimeContainer::Resume(uint32_t aType)
{
  if (!mPauseState)
    return;

  mPauseState &= ~aType;

  if (!mPauseState) {
    nsSMILTime extraOffset = GetParentTime() - mPauseStart;
    mParentOffset += extraOffset;
    NotifyTimeChange();
  }
}

nsSMILTime
nsSMILTimeContainer::GetCurrentTime() const
{
  
  
  
  
  
  if (IsPausedByType(PAUSE_BEGIN))
    return 0L;

  return mCurrentTime;
}

void
nsSMILTimeContainer::SetCurrentTime(nsSMILTime aSeekTo)
{
  
  
  aSeekTo = std::max<nsSMILTime>(0, aSeekTo);

  
  
  
  
  
  nsSMILTime parentTime = GetParentTime();
  mParentOffset = parentTime - aSeekTo;
  mIsSeeking = true;

  if (IsPaused()) {
    mNeedsPauseSample = true;
    mPauseStart = parentTime;
  }

  if (aSeekTo < mCurrentTime) {
    
    mNeedsRewind = true;
    ClearMilestones();
  }

  
  
  UpdateCurrentTime();

  NotifyTimeChange();
}

nsSMILTime
nsSMILTimeContainer::GetParentTime() const
{
  if (mParent)
    return mParent->GetCurrentTime();

  return 0L;
}

void
nsSMILTimeContainer::SyncPauseTime()
{
  if (IsPaused()) {
    nsSMILTime parentTime = GetParentTime();
    nsSMILTime extraOffset = parentTime - mPauseStart;
    mParentOffset += extraOffset;
    mPauseStart = parentTime;
  }
}

void
nsSMILTimeContainer::Sample()
{
  if (!NeedsSample())
    return;

  UpdateCurrentTime();
  DoSample();

  mNeedsPauseSample = false;
}

nsresult
nsSMILTimeContainer::SetParent(nsSMILTimeContainer* aParent)
{
  if (mParent) {
    mParent->RemoveChild(*this);
    
    
    
    
    
    
    mParentOffset = -mCurrentTime;
    mPauseStart = 0L;
  }

  mParent = aParent;

  nsresult rv = NS_OK;
  if (mParent) {
    rv = mParent->AddChild(*this);
  }

  return rv;
}

bool
nsSMILTimeContainer::AddMilestone(const nsSMILMilestone& aMilestone,
                                  mozilla::dom::SVGAnimationElement& aElement)
{
  
  
  
  
  return mMilestoneEntries.Push(MilestoneEntry(aMilestone, aElement));
}

void
nsSMILTimeContainer::ClearMilestones()
{
  mMilestoneEntries.Clear();
}

bool
nsSMILTimeContainer::GetNextMilestoneInParentTime(
    nsSMILMilestone& aNextMilestone) const
{
  if (mMilestoneEntries.IsEmpty())
    return false;

  nsSMILTimeValue parentTime =
    ContainerToParentTime(mMilestoneEntries.Top().mMilestone.mTime);
  if (!parentTime.IsDefinite())
    return false;

  aNextMilestone = nsSMILMilestone(parentTime.GetMillis(),
                                   mMilestoneEntries.Top().mMilestone.mIsEnd);

  return true;
}

bool
nsSMILTimeContainer::PopMilestoneElementsAtMilestone(
      const nsSMILMilestone& aMilestone,
      AnimElemArray& aMatchedElements)
{
  if (mMilestoneEntries.IsEmpty())
    return false;

  nsSMILTimeValue containerTime = ParentToContainerTime(aMilestone.mTime);
  if (!containerTime.IsDefinite())
    return false;

  nsSMILMilestone containerMilestone(containerTime.GetMillis(),
                                     aMilestone.mIsEnd);

  NS_ABORT_IF_FALSE(mMilestoneEntries.Top().mMilestone >= containerMilestone,
      "Trying to pop off earliest times but we have earlier ones that were "
      "overlooked");

  bool gotOne = false;
  while (!mMilestoneEntries.IsEmpty() &&
      mMilestoneEntries.Top().mMilestone == containerMilestone)
  {
    aMatchedElements.AppendElement(mMilestoneEntries.Pop().mTimebase);
    gotOne = true;
  }

  return gotOne;
}

void
nsSMILTimeContainer::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  const MilestoneEntry* p = mMilestoneEntries.Elements();
  while (p < mMilestoneEntries.Elements() + mMilestoneEntries.Length()) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback, "mTimebase");
    aCallback->NoteXPCOMChild(static_cast<nsIContent*>(p->mTimebase.get()));
    ++p;
  }
}

void
nsSMILTimeContainer::Unlink()
{
  mMilestoneEntries.Clear();
}

void
nsSMILTimeContainer::UpdateCurrentTime()
{
  nsSMILTime now = IsPaused() ? mPauseStart : GetParentTime();
  mCurrentTime = now - mParentOffset;
  NS_ABORT_IF_FALSE(mCurrentTime >= 0, "Container has negative time");
}

void
nsSMILTimeContainer::NotifyTimeChange()
{
  
  
  
  
  
  
  
  
  
  const MilestoneEntry* p = mMilestoneEntries.Elements();
#if DEBUG
  uint32_t queueLength = mMilestoneEntries.Length();
#endif
  while (p < mMilestoneEntries.Elements() + mMilestoneEntries.Length()) {
    mozilla::dom::SVGAnimationElement* elem = p->mTimebase.get();
    elem->TimedElement().HandleContainerTimeChange();
    NS_ABORT_IF_FALSE(queueLength == mMilestoneEntries.Length(),
        "Call to HandleContainerTimeChange resulted in a change to the "
        "queue of milestones");
    ++p;
  }
}
