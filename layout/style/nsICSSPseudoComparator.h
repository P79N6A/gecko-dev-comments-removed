






































#ifndef nsICSSPseudoComparator_h___
#define nsICSSPseudoComparator_h___


#define NS_ICSS_PSEUDO_COMPARATOR_IID     \
{ 0x4b122120, 0xf2d, 0x4e88, { 0xaf, 0xe9, 0x84, 0xa9, 0xae, 0x24, 0x4, 0xe5 } }

class nsIAtom;
struct nsCSSSelector;

class nsICSSPseudoComparator: public nsISupports {
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ICSS_PSEUDO_COMPARATOR_IID)

  NS_IMETHOD  PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsICSSPseudoComparator,
                              NS_ICSS_PSEUDO_COMPARATOR_IID)

#endif 
