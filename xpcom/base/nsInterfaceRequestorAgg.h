



#ifndef nsInterfaceRequestorAgg_h__
#define nsInterfaceRequestorAgg_h__

#include "nsError.h"

class nsIEventTarget;
class nsIInterfaceRequestor;









extern nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor* aFirst,
                                    nsIInterfaceRequestor* aSecond,
                                    nsIInterfaceRequestor** aResult);





extern nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor* aFirst,
                                    nsIInterfaceRequestor* aSecond,
                                    nsIEventTarget* aTarget,
                                    nsIInterfaceRequestor** aResult);

#endif 
