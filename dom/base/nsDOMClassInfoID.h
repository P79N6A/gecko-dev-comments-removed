










































#ifndef nsDOMClassInfoID_h__
#define nsDOMClassInfoID_h__

#define DOMCI_CLASS(_dom_class)                                               \
  eDOMClassInfo_##_dom_class##_id,

enum nsDOMClassInfoID {

#include "nsDOMClassInfoClasses.h"

  
  eDOMClassInfoIDCount
};

#undef DOMCI_CLASS

















#define DOMCI_CASTABLE_INTERFACES(_extra)                                     \
DOMCI_CASTABLE_INTERFACE(nsINode, 0, _extra)                                  \
DOMCI_CASTABLE_INTERFACE(nsIContent, 1, _extra)                               \
DOMCI_CASTABLE_INTERFACE(nsIDocument, 2, _extra)                              \
DOMCI_CASTABLE_INTERFACE(nsINodeList, 3, _extra)                              \
DOMCI_CASTABLE_INTERFACE(nsICSSDeclaration, 4, _extra)


#ifdef _IMPL_NS_LAYOUT

#define DOMCI_CLASS(_dom_class)                                               \
  extern const PRUint32 kDOMClassInfo_##_dom_class##_interfaces;

#include "nsDOMClassInfoClasses.h"

#undef DOMCI_CLASS








#define DOMCI_CASTABLE_INTERFACE(_interface, _bit, _extra)                    \
class _interface;                                                             \
inline PRUint32 Implements_##_interface(_interface *foo)                      \
  { return 1 << _bit; }                                                       \
inline PRUint32 Implements_##_interface(void *foo)                            \
  { return 0; }

DOMCI_CASTABLE_INTERFACES()

#undef DOMCI_CASTABLE_INTERFACE








#define DOMCI_CASTABLE_INTERFACE(_interface, _bit, _class)                    \
  Implements_##_interface((_class*)nsnull) +

#define DOMCI_DATA(_dom_class, _class)                                        \
const PRUint32 kDOMClassInfo_##_dom_class##_interfaces =                      \
  DOMCI_CASTABLE_INTERFACES(_class)                                           \
  0;

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
