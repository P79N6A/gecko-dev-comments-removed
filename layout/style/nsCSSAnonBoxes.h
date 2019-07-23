







































#ifndef nsCSSAnonBoxes_h___
#define nsCSSAnonBoxes_h___

#include "nsIAtom.h"



class nsICSSAnonBoxPseudo : public nsIAtom {};

class nsCSSAnonBoxes {
public:

  static void AddRefAtoms();

  static PRBool IsAnonBox(nsIAtom *aAtom);

#define CSS_ANON_BOX(_name, _value) static nsICSSAnonBoxPseudo* _name;
#include "nsCSSAnonBoxList.h"
#undef CSS_ANON_BOX
};

#endif 
