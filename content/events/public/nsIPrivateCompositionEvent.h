




































#ifndef nsIPrivateCompositionEvent_h__
#define nsIPrivateCompositionEvent_h__

#include "nsEvent.h"
#include "nsISupports.h"


#define NS_IPRIVATECOMPOSITIONEVENT_IID	\
{ 0x889792dc, 0x22d8, 0x4d1a, \
{ 0xac, 0x3d, 0x58, 0xad, 0x7d, 0xeb, 0xa1, 0x7b }}

class nsIPrivateCompositionEvent : public nsISupports {

public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATECOMPOSITIONEVENT_IID)

  NS_IMETHOD GetCompositionReply(struct nsTextEventReply** aReply) = 0;
  NS_IMETHOD GetReconversionReply(nsReconversionEventReply** aReply) = 0;
  NS_IMETHOD GetQueryCaretRectReply(nsQueryCaretRectEventReply** aReply) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateCompositionEvent,
                              NS_IPRIVATECOMPOSITIONEVENT_IID)

#endif 

