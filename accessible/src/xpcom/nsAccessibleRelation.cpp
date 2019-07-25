




#include "nsAccessibleRelation.h"

#include "Relation.h"
#include "Accessible.h"

#include "nsArrayUtils.h"
#include "nsComponentManagerUtils.h"

using namespace mozilla::a11y;

nsAccessibleRelation::nsAccessibleRelation(PRUint32 aType,
                                           Relation* aRel) :
  mType(aType)
{
  mTargets = do_CreateInstance(NS_ARRAY_CONTRACTID);
  nsIAccessible* targetAcc = nullptr;
  while ((targetAcc = aRel->Next()))
    mTargets->AppendElement(targetAcc, false);
}


NS_IMPL_ISUPPORTS1(nsAccessibleRelation, nsIAccessibleRelation)


NS_IMETHODIMP
nsAccessibleRelation::GetRelationType(PRUint32 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = mType;
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibleRelation::GetTargetsCount(PRUint32 *aCount)
{
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;
  return mTargets->GetLength(aCount);
}

NS_IMETHODIMP
nsAccessibleRelation::GetTarget(PRUint32 aIndex, nsIAccessible **aTarget)
{
  NS_ENSURE_ARG_POINTER(aTarget);
  nsresult rv = NS_OK;
  nsCOMPtr<nsIAccessible> target = do_QueryElementAt(mTargets, aIndex, &rv);
  target.forget(aTarget);
  return rv;
}

NS_IMETHODIMP
nsAccessibleRelation::GetTargets(nsIArray **aTargets)
{
  NS_ENSURE_ARG_POINTER(aTargets);
  NS_ADDREF(*aTargets = mTargets);
  return NS_OK;
}
