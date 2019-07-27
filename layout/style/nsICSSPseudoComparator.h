






#ifndef nsICSSPseudoComparator_h___
#define nsICSSPseudoComparator_h___

struct nsCSSSelector;

class nsICSSPseudoComparator
{
public:
  virtual bool PseudoMatches(nsCSSSelector* aSelector)=0;
};

#endif 
