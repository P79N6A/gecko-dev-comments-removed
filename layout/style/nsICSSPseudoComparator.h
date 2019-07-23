






































#ifndef nsICSSPseudoComparator_h___
#define nsICSSPseudoComparator_h___

#include "nsQueryFrame.h"

class nsIAtom;
struct nsCSSSelector;

class nsICSSPseudoComparator
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsICSSPseudoComparator)

  NS_IMETHOD  PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult)=0;
};

#endif 
