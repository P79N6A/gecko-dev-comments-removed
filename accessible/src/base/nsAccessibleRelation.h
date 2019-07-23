





































#ifndef _nsAccessibleRelation_H_
#define _nsAccessibleRelation_H_

#include "nsIAccessible.h"
#include "nsIAccessibleRelation.h"

#include "nsCOMPtr.h"
#include "nsIMutableArray.h"

#define NS_ACCRELATION_IMPL_CID                         \
{                                                       \
  0xb20390d0,                                           \
  0x40d3,                                               \
  0x4c76,                                               \
  { 0xb6, 0x2e, 0xc2, 0x30, 0xc8, 0xea, 0x0c, 0x1e }    \
}




class nsAccessibleRelation: public nsIAccessibleRelation
{
public:
  nsAccessibleRelation(PRUint32 aType, nsIAccessible *aTarget);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLERELATION

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ACCRELATION_IMPL_CID)

  




  nsresult AddTarget(nsIAccessible *aTarget);

private:
  PRUint32 mType;
  nsCOMPtr<nsIMutableArray> mTargets;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsAccessibleRelation, NS_ACCRELATION_IMPL_CID)

#endif
