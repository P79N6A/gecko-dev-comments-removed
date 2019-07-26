




#ifndef _a11yGeneric_H_
#define _a11yGeneric_H_

#include "nsThreadUtils.h"






#define NS_INTERFACE_MAP_STATIC_AMBIGUOUS(_class)                              \
  if (aIID.Equals(NS_GET_IID(_class))) {                                       \
    NS_ADDREF(this);                                                           \
    *aInstancePtr = this;                                                      \
    return NS_OK;                                                              \
  } else

#endif
