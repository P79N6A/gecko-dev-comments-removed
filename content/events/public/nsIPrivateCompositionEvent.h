




































#ifndef nsIPrivateCompositionEvent_h__
#define nsIPrivateCompositionEvent_h__

#include "nsEvent.h"
#include "nsISupports.h"


#define NS_IPRIVATECOMPOSITIONEVENT_IID	\
{ 0x901b82d5, 0x67c0, 0x45ad, \
{ 0x86, 0xae, 0xab, 0x9a, 0x6b, 0xd7, 0x41, 0x11 } }

class nsIPrivateCompositionEvent : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATECOMPOSITIONEVENT_IID)

  NS_IMETHOD GetCompositionReply(struct nsTextEventReply** aReply) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateCompositionEvent,
                              NS_IPRIVATECOMPOSITIONEVENT_IID)

#endif 

