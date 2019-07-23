







































#ifndef nsCSSPseudoElements_h___
#define nsCSSPseudoElements_h___

#include "nsIAtom.h"



class nsICSSPseudoElement : public nsIAtom {};

class nsCSSPseudoElements {
public:

  static void AddRefAtoms();

  static PRBool IsPseudoElement(nsIAtom *aAtom);

  static PRBool IsCSS2PseudoElement(nsIAtom *aAtom);

#define CSS_PSEUDO_ELEMENT(_name, _value) static nsICSSPseudoElement* _name;
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT
};

#endif 
