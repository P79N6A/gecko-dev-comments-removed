









#ifndef nsDOMClassInfoID_h__
#define nsDOMClassInfoID_h__

#include "nsIXPCScriptable.h"

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
DOMCI_CASTABLE_NODECL_INTERFACE(mozilla::dom::Element,  mozilla::dom::Element,\
                                1, _extra)                                    \
/* If this is ever removed, the IID for EventTarget can go away */            \
DOMCI_CASTABLE_NODECL_INTERFACE(mozilla::dom::EventTarget,                    \
                                mozilla::dom::EventTarget, 2, _extra)         \
DOMCI_CASTABLE_INTERFACE(nsDOMEvent, nsIDOMEvent, 3, _extra)                  \
DOMCI_CASTABLE_INTERFACE(nsIDocument, nsIDocument, 4, _extra)                 \
DOMCI_CASTABLE_INTERFACE(nsDocument, nsIDocument, 5, _extra)                  \
DOMCI_CASTABLE_INTERFACE(nsGenericHTMLElement, nsIContent, 6, _extra)         \
DOMCI_CASTABLE_INTERFACE(nsHTMLDocument, nsIDocument, 7, _extra)              \
DOMCI_CASTABLE_INTERFACE(nsStyledElement, nsStyledElement, 8, _extra)         \
DOMCI_CASTABLE_INTERFACE(nsSVGElement, nsIContent, 9, _extra)                 \
/* NOTE: When removing the casts below, remove the nsDOMEventBase class */    \
DOMCI_CASTABLE_NODECL_INTERFACE(mozilla::dom::MouseEvent,                     \
                                nsDOMEventBase, 10, _extra)                   \
DOMCI_CASTABLE_NODECL_INTERFACE(mozilla::dom::UIEvent,                        \
                                nsDOMEventBase, 11, _extra)                   \
DOMCI_CASTABLE_INTERFACE(nsGlobalWindow, nsIDOMEventTarget, 12, _extra)



#define DOMCI_CASTABLE_NODECL_INTERFACE(_interface, _u1, _u2, _u3)
#define DOMCI_CASTABLE_INTERFACE(_interface, _u1, _u2, _u3) class _interface;
DOMCI_CASTABLE_INTERFACES(unused)
#undef DOMCI_CASTABLE_INTERFACE
#undef DOMCI_CASTABLE_NODECL_INTERFACE
namespace mozilla {
namespace dom {
class Element;
class EventTarget;
class MouseEvent;
class UIEvent;
} 
} 

#define DOMCI_CASTABLE_NODECL_INTERFACE DOMCI_CASTABLE_INTERFACE

#ifdef MOZILLA_INTERNAL_API

#define DOMCI_CLASS(_dom_class)                                               \
  extern const uint32_t kDOMClassInfo_##_dom_class##_interfaces;

#include "nsDOMClassInfoClasses.h"

#undef DOMCI_CLASS







#if defined(__GNUC__) || _MSC_FULL_VER >= 140050215



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
const uint32_t kDOMClassInfo_##_dom_class##_interfaces =                      \
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
      *aInstancePtr = nullptr;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#define NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO_CONDITIONAL(_class, condition)   \
  if ((condition) &&                                                          \
      (aIID.Equals(NS_GET_IID(nsIClassInfo)) ||                               \
       aIID.Equals(NS_GET_IID(nsXPCClassInfo)))) {                            \
    foundInterface = NS_GetDOMClassInfoInstance(eDOMClassInfo_##_class##_id); \
    if (!foundInterface) {                                                    \
      *aInstancePtr = nullptr;                                                 \
      return NS_ERROR_OUT_OF_MEMORY;                                          \
    }                                                                         \
  } else

#else



#endif 

#endif 
