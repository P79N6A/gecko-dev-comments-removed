





































 

#ifndef _NS_EVENTHANDLER_H_
#define _NS_EVENTHANDLER_H_

#include <MacTypes.h>
#include <Events.h>

class nsEventHandler
{

public:
	nsEventHandler();
	~nsEventHandler();
	
	OSErr	HandleNextEvent(EventRecord *aEvt);
	OSErr	HandleMouseDown();
	OSErr	HandleKeyDown();
	OSErr	HandleUpdateEvt();
	OSErr	HandleActivateEvt();
	OSErr	HandleOSEvt();
	OSErr	HandleInContent();
	OSErr	HandleMenuChoice(SInt32 aChoice);
	
private:
	EventRecord	*mCurrEvent;
};

#endif 