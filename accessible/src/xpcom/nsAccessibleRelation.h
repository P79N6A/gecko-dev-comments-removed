




#ifndef _nsAccessibleRelation_H_
#define _nsAccessibleRelation_H_

#include "nsIAccessibleRelation.h"

#include "nsCOMPtr.h"
#include "nsIMutableArray.h"
#include "mozilla/Attributes.h"

namespace mozilla {
namespace a11y {

class Relation;




class nsAccessibleRelation MOZ_FINAL : public nsIAccessibleRelation
{
public:
  nsAccessibleRelation(uint32_t aType, Relation* aRel);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIACCESSIBLERELATION

private:
  nsAccessibleRelation();
  nsAccessibleRelation(const nsAccessibleRelation&);
  nsAccessibleRelation& operator = (const nsAccessibleRelation&);
  
  uint32_t mType;
  nsCOMPtr<nsIMutableArray> mTargets;
};

} 
} 

#endif
