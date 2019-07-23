




































#include "nsSMILTimeValueSpec.h"
#include "nsSMILInterval.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILTimeValue.h"
#include "nsSMILTimedElement.h"
#include "nsSMILInstanceTime.h"
#include "nsSMILParserUtils.h"
#include "nsISMILAnimationElement.h"
#include "nsContentUtils.h"
#include "nsString.h"




namespace
{
  
  
  class AutoBoolSetter
  {
  public:
    AutoBoolSetter(PRPackedBool& aValue)
    : mValue(aValue)
    {
      mValue = PR_TRUE;
    }

    ~AutoBoolSetter()
    {
      mValue = PR_FALSE;
    }

  private:
    PRPackedBool&   mValue;
  };
}




#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSMILTimeValueSpec::nsSMILTimeValueSpec(nsSMILTimedElement& aOwner,
                                         PRBool aIsBegin)
  : mOwner(&aOwner),
    mIsBegin(aIsBegin),
    mVisited(PR_FALSE),
    mChainEnd(PR_FALSE),
    mTimebase(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
}

nsSMILTimeValueSpec::~nsSMILTimeValueSpec()
{
  UnregisterFromTimebase(GetTimebaseElement());
}

nsresult
nsSMILTimeValueSpec::SetSpec(const nsAString& aStringSpec,
                             nsIContent* aContextNode)
{
  nsSMILTimeValueSpecParams params;
  nsresult rv =
    nsSMILParserUtils::ParseTimeValueSpecParams(aStringSpec, params);

  if (NS_FAILED(rv))
    return rv;

  
  
  
  
  NS_ABORT_IF_FALSE(!mLatestInstanceTime,
      "Attempting to re-use nsSMILTimeValueSpec object. "
      "Last instance time is non-null");

  mParams = params;

  
  
  
  
  if (mParams.mType == nsSMILTimeValueSpecParams::OFFSET ||
      (!mIsBegin && mParams.mType == nsSMILTimeValueSpecParams::INDEFINITE)) {
    nsRefPtr<nsSMILInstanceTime> instance =
      new nsSMILInstanceTime(mParams.mOffset, nsnull);
    if (!instance)
      return NS_ERROR_OUT_OF_MEMORY;
    mOwner->AddInstanceTime(instance, mIsBegin);
  }

  ResolveReferences(aContextNode);

  return rv;
}

void
nsSMILTimeValueSpec::ResolveReferences(nsIContent* aContextNode)
{
  if (mParams.mType != nsSMILTimeValueSpecParams::SYNCBASE)
    return;

  NS_ABORT_IF_FALSE(aContextNode,
      "null context node for resolving timing references against");

  
  
  if (!aContextNode->IsInDoc())
    return;

  
  
  nsRefPtr<nsIContent> oldTimebaseContent = mTimebase.get();

  NS_ABORT_IF_FALSE(mParams.mDependentElemID, "NULL syncbase element id");

  nsString idStr;
  mParams.mDependentElemID->ToString(idStr);
  mTimebase.ResetWithID(aContextNode, idStr);
  UpdateTimebase(oldTimebaseContent, mTimebase.get());
}

void
nsSMILTimeValueSpec::HandleNewInterval(const nsSMILInterval& aInterval,
                                       const nsSMILTimeContainer* aSrcContainer)
{
  NS_ABORT_IF_FALSE(aInterval.IsSet(),
      "Received notification of new interval that is not set");

  const nsSMILInstanceTime& baseInstance = mParams.mSyncBegin
    ? *aInterval.Begin() : *aInterval.End();
  nsSMILTimeValue newTime =
    ConvertBetweenTimeContainers(baseInstance.Time(), aSrcContainer);

  
  
  
  if (mIsBegin && !newTime.IsResolved())
    return;

  
  if (newTime.IsResolved()) {
    newTime.SetMillis(newTime.GetMillis() + mParams.mOffset.GetMillis());
  }

  nsRefPtr<nsSMILInstanceTime> newInstance =
    new nsSMILInstanceTime(newTime, &baseInstance,
                           nsSMILInstanceTime::SOURCE_SYNCBASE);
  if (!newInstance)
    return;

  if (mLatestInstanceTime) {
    mLatestInstanceTime->MarkNoLongerUpdating();
  }

  mLatestInstanceTime = newInstance;
  mChainEnd = PR_FALSE;
  mOwner->AddInstanceTime(newInstance, mIsBegin);
}

void
nsSMILTimeValueSpec::HandleChangedInterval(const nsSMILInterval& aInterval,
   const nsSMILTimeContainer* aSrcContainer)
{
  NS_ABORT_IF_FALSE(aInterval.IsSet(),
      "Received notification of changed interval that is not set");

  if (mVisited || mChainEnd) {
    
    
    
    
    mChainEnd = PR_TRUE;
    return;
  }

  AutoBoolSetter setVisited(mVisited);

  
  
  
  
  
  if (!mLatestInstanceTime) {
    HandleNewInterval(aInterval, aSrcContainer);
    return;
  }

  const nsSMILInstanceTime& baseInstance = mParams.mSyncBegin
    ? *aInterval.Begin() : *aInterval.End();
  NS_ABORT_IF_FALSE(mLatestInstanceTime != &baseInstance,
      "Instance time is dependent on itself");

  nsSMILTimeValue updatedTime =
    ConvertBetweenTimeContainers(baseInstance.Time(), aSrcContainer);

  
  if (mIsBegin && !updatedTime.IsResolved()) {
    HandleDeletedInterval();
    return;
  }

  
  if (updatedTime.IsResolved()) {
    updatedTime.SetMillis(updatedTime.GetMillis() +
                          mParams.mOffset.GetMillis());
  }

  
  
  
  
  
  
  
  
  
  
  
  if (!mLatestInstanceTime->MayUpdate())
    return;

  
  
  if (mLatestInstanceTime->Time() != updatedTime ||
      mLatestInstanceTime->GetDependentTime() != &baseInstance) {
    mOwner->UpdateInstanceTime(mLatestInstanceTime, updatedTime,
                               &baseInstance, mIsBegin);
  }
}

void
nsSMILTimeValueSpec::HandleDeletedInterval()
{
  
  
  
  if (!mLatestInstanceTime)
    return;

  
  
  
  nsRefPtr<nsSMILInstanceTime> oldInstanceTime = mLatestInstanceTime;
  mLatestInstanceTime = nsnull;
  mChainEnd = PR_FALSE;

  mOwner->RemoveInstanceTime(oldInstanceTime, mIsBegin);
}

void
nsSMILTimeValueSpec::Traverse(nsCycleCollectionTraversalCallback* aCallback)
{
  mTimebase.Traverse(aCallback);
}

void
nsSMILTimeValueSpec::Unlink()
{
  UnregisterFromTimebase(GetTimebaseElement());
  mTimebase.Unlink();
}




void
nsSMILTimeValueSpec::UpdateTimebase(nsIContent* aFrom, nsIContent* aTo)
{
  if (aFrom == aTo)
    return;

  UnregisterFromTimebase(GetTimedElementFromContent(aFrom));

  nsSMILTimedElement* to = GetTimedElementFromContent(aTo);
  if (to) {
    to->AddDependent(*this);
  }
}

void
nsSMILTimeValueSpec::UnregisterFromTimebase(nsSMILTimedElement* aTimedElement)
{
  if (!aTimedElement)
    return;

  aTimedElement->RemoveDependent(*this);
  HandleDeletedInterval();
}

nsSMILTimedElement*
nsSMILTimeValueSpec::GetTimedElementFromContent(nsIContent* aContent)
{
  if (!aContent)
    return nsnull;

  nsCOMPtr<nsISMILAnimationElement> animElement = do_QueryInterface(aContent);
  if (!animElement)
    return nsnull;

  return &animElement->TimedElement();
}

nsSMILTimedElement*
nsSMILTimeValueSpec::GetTimebaseElement()
{
  return GetTimedElementFromContent(mTimebase.get());
}

nsSMILTimeValue
nsSMILTimeValueSpec::ConvertBetweenTimeContainers(
    const nsSMILTimeValue& aSrcTime,
    const nsSMILTimeContainer* aSrcContainer)
{
  
  
  if (!aSrcTime.IsResolved())
    return aSrcTime;

  
  const nsSMILTimeContainer* dstContainer = mOwner->GetTimeContainer();
  if (dstContainer == aSrcContainer)
    return aSrcTime;

  
  
  if (!aSrcContainer || !dstContainer)
    return nsSMILTimeValue(); 

  nsSMILTimeValue docTime =
    aSrcContainer->ContainerToParentTime(aSrcTime.GetMillis());

  if (docTime.IsIndefinite())
    
    
    return docTime;

   NS_ABORT_IF_FALSE(docTime.IsResolved(),
       "ContainerToParentTime gave us an unresolved time");

  return dstContainer->ParentToContainerTime(docTime.GetMillis());
}
