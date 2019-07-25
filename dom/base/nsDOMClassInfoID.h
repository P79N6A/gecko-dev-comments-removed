










































#ifndef nsDOMClassInfoID_h__
#define nsDOMClassInfoID_h__

#define DOMCI_CLASS(_dom_class)                                               \
  eDOMClassInfo_##_dom_class##_id,

enum nsDOMClassInfoID {

#include "nsDOMClassInfoClasses.h"

  
  eDOMClassInfoIDCount
};

#undef DOMCI_CLASS

















#undef DOMCI_CASTABLE_INTERFACE
#define DOMCI_CASTABLE_INTERFACES(_extra)                                     \
DOMCI_CASTABLE_INTERFACE(nsINode, nsINode, 0, _extra)                         \
DOMCI_CASTABLE_INTERFACE(nsIContent, nsIContent, 1, _extra)                   \
DOMCI_CASTABLE_INTERFACE(nsIDocument, nsIDocument, 2, _extra)                 \
DOMCI_CASTABLE_INTERFACE(nsINodeList, nsINodeList, 3, _extra)                 \
DOMCI_CASTABLE_INTERFACE(nsICSSDeclaration, nsICSSDeclaration, 4, _extra)     \
DOMCI_CASTABLE_INTERFACE(nsDocument, nsIDocument, 5, _extra)                  \
DOMCI_CASTABLE_INTERFACE(nsGenericHTMLElement, nsGenericHTMLElement, 6,       \
                         _extra)                                              \
DOMCI_CASTABLE_INTERFACE(nsHTMLDocument, nsIDocument, 7, _extra)              \
DOMCI_CASTABLE_INTERFACE(nsStyledElement, nsStyledElement, 8, _extra)         \
DOMCI_CASTABLE_INTERFACE(nsSVGStylableElement, nsIContent, 9, _extra)



#define DOMCI_CASTABLE_INTERFACE(_interface, _u1, _u2, _u3) class _interface;
DOMCI_CASTABLE_INTERFACES(unused)
#undef DOMCI_CASTABLE_INTERFACE

#ifdef _IMPL_NS_LAYOUT

#define DOMCI_CLASS(_dom_class)                                               \
  extern const PRUint32 kDOMClassInfo_##_dom_class##_interfaces;

#include "nsDOMClassInfoClasses.h"

#undef DOMCI_CLASS







#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2) || \
    _MSC_FULL_VER >= 140050215



#define DOMCI_CASTABLE_TO(_interface, _class) __is_base_of(_interface, _class)

#else









template <typename Interface> struct DOMCI_CastableTo {
  struct false_type { int x[1]; };
  struct true_type { int x[2]; };
  static false_type p(void*);
  static true_type p(Interface*);
};

#define DOMCI_CASTABLE_TO(_interface, _class)                                 \
  (sizeof(DOMCI_CastableTo<_interface>::p(static_cast<_class*>(0))) ==        \
   sizeof(DOMCI_CastableTo<_interface>::true_type))

#endif




#define DOMCI_CASTABLE_INTERFACE(_interface, _base, _bit, _class)             \
  (DOMCI_CASTABLE_TO(_interface, _class) ? 1 << _bit : 0) +

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

#define NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO_CONDITIONAL(_class, condition)   \
  if ((condition) &&                                                          \
      (aIID.Equals(NS_GET_IID(nsIClassInfo)) ||                               \
       aIID.Equals(NS_GET_IID(nsXPCClassInfo)))) {                            \
    foundInterface = NS_GetDOMClassInfoInstance(eDOMClassInfo_##_class##_id); \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nsnull;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#else



#endif 

#endif 
