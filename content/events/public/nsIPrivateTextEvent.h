




































#ifndef nsIPrivateTextEvent_h__
#define nsIPrivateTextEvent_h__

#include "nsEvent.h"
#include "nsISupports.h"
#include "nsIPrivateTextRange.h"

#define NS_IPRIVATETEXTEVENT_IID \
{ /* 37B69251-4ACE-11d3-9EA6-0060089FE59B */ \
0x37b69251, 0x4ace, 0x11d3, \
{0x9e, 0xa6, 0x0, 0x60, 0x8, 0x9f, 0xe5, 0x9b} }

class nsIPrivateTextEvent : public nsISupports {

public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_IPRIVATETEXTEVENT_IID)

	NS_IMETHOD GetText(nsString& aText) = 0;
	NS_IMETHOD_(already_AddRefed<nsIPrivateTextRangeList>) GetInputRange() = 0;
	NS_IMETHOD_(nsTextEventReply*) GetEventReply() = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIPrivateTextEvent, NS_IPRIVATETEXTEVENT_IID)

#endif 

