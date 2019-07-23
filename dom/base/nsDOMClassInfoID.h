










































#ifndef nsDOMClassInfoID_h__
#define nsDOMClassInfoID_h__

#define DOMCI_CLASS(_dom_class) \
  eDOMClassInfo_##_dom_class##_id,

enum nsDOMClassInfoID {

#include "nsDOMClassInfoClasses.h"

  
  eDOMClassInfoIDCount
};

#undef DOMCI_CLASS





#ifdef _IMPL_NS_LAYOUT

#define DOMCI_DATA(_dom_class, _class)

class nsIClassInfo;
class nsXPCClassInfo;

extern nsIClassInfo*
NS_GetDOMClassInfoInstance(nsDOMClassInfoID aID);

#define NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(_class)                          \
  if (aIID.Equals(NS_GET_IID(nsIClassInfo)) ||                                \
      aIID.Equals(NS_GET_IID(nsXPCClassInfo))) {                              \
    foundInterface = NS_GetDOMClassInfoInstance(eDOMClassInfo_##_class##_id); \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#else



#endif 

#endif 
