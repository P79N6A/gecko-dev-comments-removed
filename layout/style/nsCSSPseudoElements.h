






#ifndef nsCSSPseudoElements_h___
#define nsCSSPseudoElements_h___

#include "nsIAtom.h"




#define CSS_PSEUDO_ELEMENT_IS_CSS2                (1<<0)








#define CSS_PSEUDO_ELEMENT_CONTAINS_ELEMENTS      (1<<1)



class nsICSSPseudoElement : public nsIAtom {};

class nsCSSPseudoElements {
public:

  static void AddRefAtoms();

  static bool IsPseudoElement(nsIAtom *aAtom);

  static bool IsCSS2PseudoElement(nsIAtom *aAtom);

  static bool PseudoElementContainsElements(nsIAtom *aAtom) {
    return PseudoElementHasFlags(aAtom, CSS_PSEUDO_ELEMENT_CONTAINS_ELEMENTS);
  }

#define CSS_PSEUDO_ELEMENT(_name, _value, _flags) \
  static nsICSSPseudoElement* _name;
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT

  enum Type {
    
    
#define CSS_PSEUDO_ELEMENT(_name, _value_, _flags) \
    ePseudo_##_name,
#include "nsCSSPseudoElementList.h"
#undef CSS_PSEUDO_ELEMENT
    ePseudo_PseudoElementCount,
    ePseudo_AnonBox = ePseudo_PseudoElementCount,
#ifdef MOZ_XUL
    ePseudo_XULTree,
#endif
    ePseudo_NotPseudoElement,
    ePseudo_MAX
  };

  static Type GetPseudoType(nsIAtom* aAtom);

  
  static nsIAtom* GetPseudoAtom(Type aType);

private:
  static PRUint32 FlagsForPseudoElement(nsIAtom *aAtom);

  
  static bool PseudoElementHasFlags(nsIAtom *aAtom, PRUint32 aFlags)
  {
    return (FlagsForPseudoElement(aAtom) & aFlags) == aFlags;
  }
};

#endif 
