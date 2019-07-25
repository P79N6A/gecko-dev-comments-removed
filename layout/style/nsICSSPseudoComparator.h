






































#ifndef nsICSSPseudoComparator_h___
#define nsICSSPseudoComparator_h___

#include "nsQueryFrame.h"

class nsIAtom;
struct nsCSSSelector;

class nsICSSPseudoComparator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsICSSPseudoComparator)

  virtual PRBool PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector)=0;
};

#endif 
