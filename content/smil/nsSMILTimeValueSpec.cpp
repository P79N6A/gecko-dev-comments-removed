




































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




#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSMILTimeValueSpec::nsSMILTimeValueSpec(nsSMILTimedElement& aOwner,
                                         PRBool aIsBegin)
  : mOwner(&aOwner),
    mIsBegin(aIsBegin),
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

  mParams = params;

  
  
  
  
  if (mParams.mType == nsSMILTimeValueSpecParams::OFFSET ||
      (!mIsBegin && mParams.mType == nsSMILTimeValueSpecParams::INDEFINITE)) {
    nsRefPtr<nsSMILInstanceTime> instance =
      new nsSMILInstanceTime(mParams.mOffset);
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
nsSMILTimeValueSpec::HandleNewInterval(nsSMILInterval& aInterval,
                                       const nsSMILTimeContainer* aSrcContainer)
{
  const nsSMILInstanceTime& baseInstance = mParams.mSyncBegin
    ? *aInterval.Begin() : *aInterval.End();
  nsSMILTimeValue newTime =
    ConvertBetweenTimeContainers(baseInstance.Time(), aSrcContainer);

  
  if (newTime.IsResolved()) {
    newTime.SetMillis(newTime.GetMillis() + mParams.mOffset.GetMillis());
  }

  
  nsRefPtr<nsSMILInstanceTime> newInstance =
    new nsSMILInstanceTime(newTime, nsSMILInstanceTime::SOURCE_SYNCBASE, this,
                           &aInterval);
  if (!newInstance)
    return;

  mOwner->AddInstanceTime(newInstance, mIsBegin);
}

void
nsSMILTimeValueSpec::HandleChangedInstanceTime(
    const nsSMILInstanceTime& aBaseTime,
    const nsSMILTimeContainer* aSrcContainer,
    nsSMILInstanceTime& aInstanceTimeToUpdate,
    PRBool aObjectChanged)
{
  
  
  if (aInstanceTimeToUpdate.IsFixedTime())
    return;

  nsSMILTimeValue updatedTime =
    ConvertBetweenTimeContainers(aBaseTime.Time(), aSrcContainer);

  
  if (updatedTime.IsResolved()) {
    updatedTime.SetMillis(updatedTime.GetMillis() +
                          mParams.mOffset.GetMillis());
  }

  
  
  if (aInstanceTimeToUpdate.Time() != updatedTime || aObjectChanged) {
    mOwner->UpdateInstanceTime(&aInstanceTimeToUpdate, updatedTime, mIsBegin);
  }
}

void
nsSMILTimeValueSpec::HandleDeletedInstanceTime(
    nsSMILInstanceTime &aInstanceTime)
{
  mOwner->RemoveInstanceTime(&aInstanceTime, mIsBegin);
}

PRBool
nsSMILTimeValueSpec::DependsOnBegin() const
{
  return mParams.mSyncBegin;
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
  mOwner->RemoveInstanceTimesForCreator(this, mIsBegin);
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
