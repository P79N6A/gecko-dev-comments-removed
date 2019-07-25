



































#ifndef nsTransportUtils_h__
#define nsTransportUtils_h__

#include "nsITransport.h"














NS_HIDDEN_(nsresult)
net_NewTransportEventSinkProxy(nsITransportEventSink **aResult,
                               nsITransportEventSink *aSink,
                               nsIEventTarget *aTarget,
                               PRBool aCoalesceAllEvents = PR_FALSE);

#endif 
