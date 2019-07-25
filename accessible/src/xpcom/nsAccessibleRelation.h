





































#ifndef _nsAccessibleRelation_H_
#define _nsAccessibleRelation_H_

#include "nsIAccessibleRelation.h"

#include "nsCOMPtr.h"
#include "nsIMutableArray.h"

class Relation;




class nsAccessibleRelation: public nsIAccessibleRelation
{
public:
  nsAccessibleRelation(PRUint32 aType, Relation* aRel);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLERELATION

private:
  nsAccessibleRelation();
  nsAccessibleRelation(const nsAccessibleRelation&);
  nsAccessibleRelation& operator = (const nsAccessibleRelation&);
  
  PRUint32 mType;
  nsCOMPtr<nsIMutableArray> mTargets;
};

#endif
