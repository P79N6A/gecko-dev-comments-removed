



#ifndef nsInterfaceRequestorAgg_h__
#define nsInterfaceRequestorAgg_h__

#include "nsIInterfaceRequestor.h"

class nsIEventTarget;









extern nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor  *aFirst,
                                    nsIInterfaceRequestor  *aSecond,
                                    nsIInterfaceRequestor **aResult);





extern nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor  *aFirst,
                                    nsIInterfaceRequestor  *aSecond,
                                    nsIEventTarget         *aTarget,
                                    nsIInterfaceRequestor **aResult);

#endif 
