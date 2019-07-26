



#ifndef nsTransportUtils_h__
#define nsTransportUtils_h__

#include "nsITransport.h"














nsresult
net_NewTransportEventSinkProxy(nsITransportEventSink **aResult,
                               nsITransportEventSink *aSink,
                               nsIEventTarget *aTarget,
                               bool aCoalesceAllEvents = false);

#endif 
