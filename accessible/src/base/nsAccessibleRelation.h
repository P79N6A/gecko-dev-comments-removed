





































#ifndef _nsAccessibleRelation_H_
#define _nsAccessibleRelation_H_

#include "nsIAccessible.h"
#include "nsIAccessibleRelation.h"

#include "nsCOMPtr.h"

class nsAccessibleRelation: public nsIAccessibleRelation
{
public:
  nsAccessibleRelation(PRUint32 aType, nsIAccessible *aTarget);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLERELATION

private:
  PRUint32 mType;
  nsCOMPtr<nsIAccessible> mTarget;
};

#endif
