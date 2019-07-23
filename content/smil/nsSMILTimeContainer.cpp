




































#include "nsSMILTimeContainer.h"

nsSMILTimeContainer::nsSMILTimeContainer()
:
  mParent(nsnull),
  mCurrentTime(0L),
  mParentOffset(0L),
  mPauseStart(0L),
  mNeedsPauseSample(PR_FALSE),
  mPauseState(PAUSE_BEGIN)
{
}

nsSMILTimeContainer::~nsSMILTimeContainer()
{
  if (mParent) {
    mParent->RemoveChild(*this);
  }
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
  if (!mPauseState && aType) {
    mPauseStart = GetParentTime();
    mNeedsPauseSample = PR_TRUE;
  }

  mPauseState |= aType;
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
  
  
  
  
  
  nsSMILTime parentTime = GetParentTime();
  mParentOffset = parentTime - aSeekTo;

  if (mPauseState) {
    mNeedsPauseSample = PR_TRUE;
    mPauseStart = parentTime;
  }

  
  
  UpdateCurrentTime();
}

nsSMILTime
nsSMILTimeContainer::GetParentTime() const
{
  if (mParent)
    return mParent->GetCurrentTime();

  return 0L;
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
  }

  mParent = aParent;

  nsresult rv = NS_OK;
  if (mParent) {
    rv = mParent->AddChild(*this);
  }

  return rv;
}

void
nsSMILTimeContainer::UpdateCurrentTime()
{
  nsSMILTime now = mPauseState ? mPauseStart : GetParentTime();
  mCurrentTime = now - mParentOffset;
}
