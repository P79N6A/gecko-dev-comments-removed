






#ifndef nsCSSPseudoElements_h___
#define nsCSSPseudoElements_h___

#include "nsIAtom.h"




#define CSS_PSEUDO_ELEMENT_IS_CSS2                  (1<<0)








#define CSS_PSEUDO_ELEMENT_CONTAINS_ELEMENTS        (1<<1)


#define CSS_PSEUDO_ELEMENT_SUPPORTS_STYLE_ATTRIBUTE (1<<2)



class nsICSSPseudoElement : public nsIAtom {};

class nsCSSPseudoElements {
public:

  static void AddRefAtoms();

  static bool IsPseudoElement(nsIAtom *aAtom);

  static bool IsCSS2PseudoElement(nsIAtom *aAtom);

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

  static bool PseudoElementContainsElements(const Type aType) {
    return PseudoElementHasFlags(aType, CSS_PSEUDO_ELEMENT_CONTAINS_ELEMENTS);
  }

  static bool PseudoElementSupportsStyleAttribute(nsIAtom *aAtom) {
    return PseudoElementHasFlags(GetPseudoType(aAtom), CSS_PSEUDO_ELEMENT_SUPPORTS_STYLE_ATTRIBUTE);
  }

  static bool PseudoElementSupportsStyleAttribute(const Type aType) {
    MOZ_ASSERT(aType < ePseudo_PseudoElementCount);
    return PseudoElementHasFlags(aType, CSS_PSEUDO_ELEMENT_SUPPORTS_STYLE_ATTRIBUTE);
  }

private:
  static uint32_t FlagsForPseudoElement(const Type aType);

  
  static bool PseudoElementHasFlags(const Type aType, uint32_t aFlags)
  {
    return (FlagsForPseudoElement(aType) & aFlags) == aFlags;
  }
};

#endif 
