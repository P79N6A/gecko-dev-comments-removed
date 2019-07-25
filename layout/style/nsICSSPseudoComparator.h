






































#ifndef nsICSSPseudoComparator_h___
#define nsICSSPseudoComparator_h___

class nsIAtom;
struct nsCSSSelector;

class nsICSSPseudoComparator
{
public:
  virtual PRBool PseudoMatches(nsCSSSelector* aSelector)=0;
};

#endif 
