




#ifndef _a11yGeneric_H_
#define _a11yGeneric_H_

#include "nsThreadUtils.h"






#define NS_INTERFACE_MAP_STATIC_AMBIGUOUS(_class)                              \
  if (aIID.Equals(NS_GET_IID(_class))) {                                       \
    NS_ADDREF(this);                                                           \
    *aInstancePtr = this;                                                      \
    return NS_OK;                                                              \
  } else

#define NS_ENSURE_A11Y_SUCCESS(res, ret)                                       \
  PR_BEGIN_MACRO                                                               \
    nsresult __rv = res; /* Don't evaluate |res| more than once */             \
    if (NS_FAILED(__rv)) {                                                     \
      NS_ENSURE_SUCCESS_BODY(res, ret)                                         \
      return ret;                                                              \
    }                                                                          \
    if (__rv == NS_OK_DEFUNCT_OBJECT)                                          \
      return ret;                                                              \
  PR_END_MACRO

#endif
