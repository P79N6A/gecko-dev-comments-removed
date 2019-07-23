










































#ifndef nsMacMessagePump_h__
#define nsMacMessagePump_h__

#include "prtypes.h"

#include <Events.h>

class nsToolkit;
class nsMacTSMMessagePump;

class nsMacMessagePump
{
  public:
    nsMacMessagePump(nsToolkit *aToolKit);
    virtual ~nsMacMessagePump();

    PRBool ProcessEvents(PRBool aProcessEvents);
  
    
    PRBool DispatchEvent(EventRecord *anEvent);

  protected:
    
    PRBool DoMouseDown(EventRecord &anEvent);
    PRBool DoMouseUp(EventRecord &anEvent);
    PRBool DoMouseMove(EventRecord &anEvent);

    PRBool DispatchOSEventToRaptor(EventRecord &anEvent, WindowPtr aWindow);

    static pascal OSStatus MouseClickEventHandler(
                                           EventHandlerCallRef aHandlerCallRef,
                                           EventRef            aEvent,
                                           void*               aUserData);
    static pascal OSStatus WNETransitionEventHandler(
                                           EventHandlerCallRef aHandlerCallRef,
                                           EventRef            aEvent,
                                           void*               aUserData);

  protected:
    nsToolkit*           mToolkit;
    nsMacTSMMessagePump* mTSMMessagePump;
    EventHandlerRef      mMouseClickEventHandler;
    EventHandlerRef      mWNETransitionEventHandler;
};
#endif 
