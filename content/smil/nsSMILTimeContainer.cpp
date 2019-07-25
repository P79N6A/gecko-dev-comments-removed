




































#include "nsSMILTimeContainer.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimedElement.h"

nsSMILTimeContainer::nsSMILTimeContainer()
:
  mParent(nsnull),
  mCurrentTime(0L),
  mParentOffset(0L),
  mPauseStart(0L),
  mNeedsPauseSample(PR_FALSE),
  mNeedsRewind(PR_FALSE),
  mIsSeeking(PR_FALSE),
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
    mNeedsPauseSample = PR_TRUE;
  }

  
  
  
  
  
  

  UpdateCurrentTime();
}

void
nsSMILTimeContainer::Pause(PRUint32 aType)
{
  PRBool didStartPause = PR_FALSE;

  if (!mPauseState && aType) {
    mPauseStart = GetParentTime();
    mNeedsPauseSample = PR_TRUE;
    didStartPause = PR_TRUE;
  }

  mPauseState |= aType;

  if (didStartPause) {
    NotifyTimeChange();
  }
}

void
nsSMILTimeContainer::Resume(PRUint32 aType)
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
  
  
  aSeekTo = NS_MAX<nsSMILTime>(0, aSeekTo);

  
  
  
  
  
  nsSMILTime parentTime = GetParentTime();
  mParentOffset = parentTime - aSeekTo;
  mIsSeeking = PR_TRUE;

  if (IsPaused()) {
    mNeedsPauseSample = PR_TRUE;
    mPauseStart = parentTime;
  }

  if (aSeekTo < mCurrentTime) {
    
    mNeedsRewind = PR_TRUE;
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

  mNeedsPauseSample = PR_FALSE;
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

PRBool
nsSMILTimeContainer::AddMilestone(const nsSMILMilestone& aMilestone,
                                  nsISMILAnimationElement& aElement)
{
  
  
  
  
  return mMilestoneEntries.Push(MilestoneEntry(aMilestone, aElement));
}

void
nsSMILTimeContainer::ClearMilestones()
{
  mMilestoneEntries.Clear();
}

PRBool
nsSMILTimeContainer::GetNextMilestoneInParentTime(
    nsSMILMilestone& aNextMilestone) const
{
  if (mMilestoneEntries.IsEmpty())
    return PR_FALSE;

  nsSMILTimeValue parentTime =
    ContainerToParentTime(mMilestoneEntries.Top().mMilestone.mTime);
  if (!parentTime.IsResolved())
    return PR_FALSE;

  aNextMilestone = nsSMILMilestone(parentTime.GetMillis(),
                                   mMilestoneEntries.Top().mMilestone.mIsEnd);

  return PR_TRUE;
}

PRBool
nsSMILTimeContainer::PopMilestoneElementsAtMilestone(
      const nsSMILMilestone& aMilestone,
      AnimElemArray& aMatchedElements)
{
  if (mMilestoneEntries.IsEmpty())
    return PR_FALSE;

  nsSMILTimeValue containerTime = ParentToContainerTime(aMilestone.mTime);
  if (!containerTime.IsResolved())
    return PR_FALSE;

  nsSMILMilestone containerMilestone(containerTime.GetMillis(),
                                     aMilestone.mIsEnd);

  NS_ABORT_IF_FALSE(mMilestoneEntries.Top().mMilestone >= containerMilestone,
      "Trying to pop off earliest times but we have earlier ones that were "
      "overlooked");

  PRBool gotOne = PR_FALSE;
  while (!mMilestoneEntries.IsEmpty() &&
      mMilestoneEntries.Top().mMilestone == containerMilestone)
  {
    aMatchedElements.AppendElement(mMilestoneEntries.Pop().mTimebase);
    gotOne = PR_TRUE;
  }

  return gotOne;
}

void
nsSMILTimeContainer::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  const MilestoneEntry* p = mMilestoneEntries.Elements();
  while (p < mMilestoneEntries.Elements() + mMilestoneEntries.Length()) {
    NS_CYCLE_COLLECTION_NOTE_EDGE_NAME(*aCallback, "mTimebase");
    aCallback->NoteXPCOMChild(p->mTimebase.get());
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
  PRUint32 queueLength = mMilestoneEntries.Length();
#endif
  while (p < mMilestoneEntries.Elements() + mMilestoneEntries.Length()) {
    nsISMILAnimationElement* elem = p->mTimebase.get();
    elem->TimedElement().HandleContainerTimeChange();
    NS_ABORT_IF_FALSE(queueLength == mMilestoneEntries.Length(),
        "Call to HandleContainerTimeChange resulted in a change to the "
        "queue of milestones");
    ++p;
  }
}
