





































#include "nsAccessibleRelation.h"

#include "nsArrayUtils.h"
#include "nsComponentManagerUtils.h"

nsAccessibleRelation::
  nsAccessibleRelation(PRUint32 aType, nsIAccessible *aTarget) :
  mType(aType)
{
  mTargets = do_CreateInstance(NS_ARRAY_CONTRACTID);
  if (aTarget)
    mTargets->AppendElement(aTarget, PR_FALSE);
}


NS_IMPL_ISUPPORTS2(nsAccessibleRelation, nsAccessibleRelation,
                   nsIAccessibleRelation)


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

  NS_ENSURE_TRUE(mTargets, NS_ERROR_NOT_INITIALIZED);

  return mTargets->GetLength(aCount);
}

NS_IMETHODIMP
nsAccessibleRelation::GetTarget(PRUint32 aIndex, nsIAccessible **aTarget)
{
  NS_ENSURE_ARG_POINTER(aTarget);
  *aTarget = nsnull;

  NS_ENSURE_TRUE(mTargets, NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_OK;
  nsCOMPtr<nsIAccessible> target = do_QueryElementAt(mTargets, aIndex, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  target.swap(*aTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibleRelation::GetTargets(nsIArray **aTargets)
{
  NS_ENSURE_ARG_POINTER(aTargets);
  *aTargets = nsnull;

  NS_ENSURE_TRUE(mTargets, NS_ERROR_NOT_INITIALIZED);

  NS_ADDREF(*aTargets = mTargets);
  return NS_OK;
}


nsresult
nsAccessibleRelation::AddTarget(nsIAccessible *aTarget)
{
  NS_ENSURE_ARG(aTarget);

  NS_ENSURE_TRUE(mTargets, NS_ERROR_NOT_INITIALIZED);

  return mTargets->AppendElement(aTarget, PR_FALSE);
}
