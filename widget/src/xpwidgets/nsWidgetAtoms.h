




































#ifndef nsWidgetAtoms_h___
#define nsWidgetAtoms_h___

#include "prtypes.h"
#include "nsIAtom.h"










class nsWidgetAtoms {
public:

  static void RegisterAtoms();

  






#define WIDGET_ATOM(_name, _value) static nsIAtom* _name;
#include "nsWidgetAtomList.h"
#undef WIDGET_ATOM

};

#endif 
