





































#ifndef __nsEditProperty_h__
#define __nsEditProperty_h__

#include "nsISupports.h"

class nsIAtom;
class nsString;





class nsEditProperty
{
public:

    static void RegisterAtoms();

#define EDITOR_ATOM(name_, value_) static nsIAtom* name_;
#include "nsEditPropertyAtomList.h"
#undef EDITOR_ATOM

};



#endif
