







































#ifndef nsCSSPseudoClasses_h___
#define nsCSSPseudoClasses_h___

#include "nsIAtom.h"

class nsCSSPseudoClasses {
public:

  static void AddRefAtoms();

  enum Type {
#define CSS_PSEUDO_CLASS(_name, _value) \
    ePseudoClass_##_name,
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
    ePseudoClass_Count,
    ePseudoClass_NotPseudoClass 

  };

  static Type GetPseudoType(nsIAtom* aAtom);
  static bool HasStringArg(Type aType);
  static bool HasNthPairArg(Type aType);
  static bool HasSelectorListArg(Type aType) {
    return aType == ePseudoClass_any;
  }

  
  static void PseudoTypeToString(Type aType, nsAString& aString);
};

#endif 
