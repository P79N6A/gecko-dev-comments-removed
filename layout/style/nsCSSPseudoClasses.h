







































#ifndef nsCSSPseudoClasses_h___
#define nsCSSPseudoClasses_h___

#include "nsIAtom.h"



class nsICSSPseudoClass : public nsIAtom {};

class nsCSSPseudoClasses {
public:

  static void AddRefAtoms();

  static PRBool IsPseudoClass(nsIAtom *aAtom);
  static PRBool HasStringArg(nsIAtom* aAtom);
  static PRBool HasNthPairArg(nsIAtom* aAtom);

#define CSS_PSEUDO_CLASS(_name, _value) static nsICSSPseudoClass* _name;
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS

  enum Type {
#define CSS_PSEUDO_CLASS(_name, _value) \
    ePseudoClass_##_name,
#include "nsCSSPseudoClassList.h"
#undef CSS_PSEUDO_CLASS
    ePseudoClass_Count,
    ePseudoClass_NotPseudoClass
  };

  static Type GetPseudoType(nsIAtom* aAtom);
};

#endif 
