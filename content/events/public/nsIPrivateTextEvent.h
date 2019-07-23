




































#ifndef nsIPrivateTextEvent_h__
#define nsIPrivateTextEvent_h__

#include "nsEvent.h"
#include "nsISupports.h"
#include "nsIPrivateTextRange.h"

#define NS_IPRIVATETEXTEVENT_IID \
{0xeee4a0f0, 0x7fb0, 0x494c, \
{0x80, 0xe3, 0x30, 0xd3, 0x08, 0x95, 0x15, 0x2c}}

class nsIPrivateTextEvent : public nsISupports {

public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATETEXTEVENT_IID)

	NS_IMETHOD GetText(nsString& aText) = 0;
	NS_IMETHOD_(already_AddRefed<nsIPrivateTextRangeList>) GetInputRange() = 0;
	NS_IMETHOD_(nsTextEventReply*) GetEventReply() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateTextEvent, NS_IPRIVATETEXTEVENT_IID)

#endif 

