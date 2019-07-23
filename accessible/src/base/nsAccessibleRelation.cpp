





































#include "nsAccessibleRelation.h"

#include "nsIMutableArray.h"
#include "nsComponentManagerUtils.h"

nsAccessibleRelation::
  nsAccessibleRelation(PRUint32 aType, nsIAccessible *aTarget) :
  mType(aType), mTarget(aTarget)
{
}


NS_IMPL_ISUPPORTS1(nsAccessibleRelation, nsIAccessibleRelation);



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

  *aCount = 1;
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibleRelation::GetTarget(PRUint32 aIndex, nsIAccessible **aTarget)
{
  NS_ENSURE_ARG_POINTER(aTarget);

  NS_IF_ADDREF(*aTarget = mTarget);
  return NS_OK;
}

NS_IMETHODIMP
nsAccessibleRelation::GetTargets(nsIArray **aRelations)
{
  NS_ENSURE_ARG_POINTER(aRelations);

  nsCOMPtr<nsIMutableArray> relations = do_CreateInstance(NS_ARRAY_CONTRACTID);
  NS_ENSURE_TRUE(relations, NS_ERROR_OUT_OF_MEMORY);

  relations->AppendElement(mTarget, PR_FALSE);

  NS_ADDREF(*aRelations = relations);
  return NS_OK;
}
