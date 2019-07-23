







































#ifndef nsCSSPseudoElements_h___
#define nsCSSPseudoElements_h___

#include "nsIAtom.h"




#define CSS_PSEUDO_ELEMENT_IS_CSS2                (1<<0)








#define CSS_PSEUDO_ELEMENT_CONTAINS_ELEMENTS      (1<<1)



class nsICSSPseudoElement : public nsIAtom {};

class nsCSSPseudoElements {
public:

  static void AddRefAtoms();

  static PRBool IsPseudoElement(nsIAtom *aAtom);

  static PRBool IsCSS2PseudoElement(nsIAtom *aAtom) {
    return PseudoElementHasFlags(aAtom, CSS_PSEUDO_ELEMENT_IS_CSS2);
  }

  static PRBool PseudoElementContainsElements(nsIAtom *aAtom) {
    return PseudoElementHasFlags(aAtom, CSS_PSEUDO_ELEMENT_CONTAINS_ELEMENTS);
  }

#define CSS_PSEUDO_ELEMENT(_name, _value, _flags) \
  static nsICSSPseudoElement* _name;
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT

private:
  static PRUint32 FlagsForPseudoElement(nsIAtom *aAtom);

  
  static PRBool PseudoElementHasFlags(nsIAtom *aAtom, PRUint32 aFlags)
  {
    return (FlagsForPseudoElement(aAtom) & aFlags) == aFlags;
  }
};

#endif 
