










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





#ifdef MOZILLA_INTERNAL_API

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
