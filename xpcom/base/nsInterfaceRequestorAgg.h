



































#ifndef nsInterfaceRequestorAgg_h__
#define nsInterfaceRequestorAgg_h__

#include "nsIInterfaceRequestor.h"








extern NS_COM nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor  *aFirst,
                                    nsIInterfaceRequestor  *aSecond,
                                    nsIInterfaceRequestor **aResult);

#endif 
