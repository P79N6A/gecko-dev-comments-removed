















































#include <Controls.h>
#include <Events.h>

#include "nsIPluginManager2.h"
#include "EventFilter.h"
#include "nsIEventHandler.h"

#include "AltWindowHandling.h"

RegisteredWindow* theRegisteredWindows = NULL;
RegisteredWindow* theActiveWindow = NULL;
Boolean mEventFiltersInstalled = nil;

Boolean EventFilter(EventRecord* event);
Boolean MenuFilter(long menuSelection);
RegisteredWindow** GetRegisteredWindow(nsPluginPlatformWindowRef window);

NS_METHOD
AltRegisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
    theRegisteredWindows = new RegisteredWindow(theRegisteredWindows, handler, window);
    
#ifdef XP_MAC
    
    if (!mEventFiltersInstalled) {
        ::InstallEventFilters(&EventFilter, &MenuFilter);
        mEventFiltersInstalled = true;
    }

    
    
    SInt16 variant = ::GetWVariant(window);
    if (variant == plainDBox) {
        ::ShowHide(window, true);
        ::BringToFront(window);
    } else {
        ::ShowWindow(window);
        ::SelectWindow(window);
    }
#endif

    return NS_OK;
}

NS_METHOD
AltUnregisterWindow(nsIEventHandler* handler, nsPluginPlatformWindowRef window)
{
    RegisteredWindow** link = GetRegisteredWindow(window);
    if (link != NULL) {
        RegisteredWindow* registeredWindow = *link;
        if (registeredWindow == theActiveWindow)
            theActiveWindow = NULL;
        *link = registeredWindow->mNext;
        delete registeredWindow;
    }

#ifdef XP_MAC
    ::HideWindow(window);

    
    if (theRegisteredWindows == NULL) {
        ::RemoveEventFilters();
        mEventFiltersInstalled = false;
    }
#endif

    return NS_OK;
}

static void sendActivateEvent(nsIEventHandler* handler, WindowRef window, Boolean active)
{
    EventRecord event;
    ::OSEventAvail(0, &event);
    event.what = activateEvt;
    event.message = UInt32(window);
    if (active)
        event.modifiers |= activeFlag;
    else
        event.modifiers &= ~activeFlag;

    nsPluginEvent pluginEvent = { &event, window };
    PRBool handled = PR_FALSE;

    handler->HandleEvent(&pluginEvent, &handled);
}

RegisteredWindow** GetRegisteredWindow(nsPluginPlatformWindowRef window)
{
    RegisteredWindow** link = &theRegisteredWindows;
    RegisteredWindow* registeredWindow = *link;
    while (registeredWindow != NULL) {
        if (registeredWindow->mWindow == window)
            return link;
        link = &registeredWindow->mNext;
        registeredWindow = *link;
    }
    return NULL;
}
RegisteredWindow* FindRegisteredWindow(nsPluginPlatformWindowRef window);
RegisteredWindow* FindRegisteredWindow(nsPluginPlatformWindowRef window)
{
    RegisteredWindow** link = GetRegisteredWindow(window);
    return (link != NULL ? *link : NULL);
}








Boolean EventFilter(EventRecord* event)
{
    Boolean filteredEvent = false;

    WindowRef window = WindowRef(event->message);
    nsPluginEvent pluginEvent = { event, window };
    EventRecord simulatedEvent;

    RegisteredWindow* registeredWindow;
    PRBool handled = PR_FALSE;
    
    
    switch (event->what) {
    case nullEvent:
        
        
        window = ::FrontWindow();
        registeredWindow = FindRegisteredWindow(window);
        if (registeredWindow != NULL) {
            simulatedEvent = *event;
            simulatedEvent.what = nsPluginEventType_AdjustCursorEvent;
            pluginEvent.event = &simulatedEvent;
            pluginEvent.window = registeredWindow->mWindow;
            registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
        }
        break;
    case keyDown:
    case keyUp:
    case autoKey:
        
        window = ::FrontWindow();
        registeredWindow = FindRegisteredWindow(window);
        if (registeredWindow != NULL) {
            pluginEvent.window = window;
            registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
            filteredEvent = true;
        }
        break;
    case mouseDown:
        
        short partCode = FindWindow(event->where, &window);
        switch (partCode) {
        case inContent:
        case inDrag:
        case inGrow:
        case inGoAway:
        case inZoomIn:
        case inZoomOut:
        case inCollapseBox:
        case inProxyIcon:
            registeredWindow = FindRegisteredWindow(window);
            if (registeredWindow != NULL) {
                
                if (theActiveWindow == NULL) {
                    sendActivateEvent(registeredWindow->mHandler, window, true);
                    theActiveWindow = registeredWindow;
                }
                pluginEvent.window = window;
                registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
                filteredEvent = true;
            } else if (theActiveWindow != NULL) {
                
                
                
                window = theActiveWindow->mWindow;
                sendActivateEvent(theActiveWindow->mHandler, window, false);
                ::HiliteWindow(window, false);
                theActiveWindow = NULL;
            }
            break;
        }
        break;
    case activateEvt:
        registeredWindow = FindRegisteredWindow(window);
        if (registeredWindow != NULL) {
            registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
            filteredEvent = true;
            theActiveWindow = registeredWindow;
        }
        break;
    case updateEvt:
        registeredWindow = FindRegisteredWindow(window);
        if (registeredWindow != NULL) {
            GrafPtr port; GetPort(&port); SetPort(window); BeginUpdate(window);
                registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
            EndUpdate(window); SetPort(port);
            filteredEvent = true;
        }
        break;
    case osEvt:
        if ((event->message & osEvtMessageMask) == (suspendResumeMessage << 24)) {
            registeredWindow = theActiveWindow;
            if (registeredWindow != NULL) {
                window = registeredWindow->mWindow;
                Boolean active = (event->message & resumeFlag) != 0;
                sendActivateEvent(registeredWindow->mHandler, window, active);
                pluginEvent.window = window;
                registeredWindow->mHandler->HandleEvent(&pluginEvent, &handled);
                ::HiliteWindow(window, active);
            }
        }
        break;
    }
    
    return filteredEvent;
}


enum {
    kBaseMenuID = 20000,
    kBaseSubMenuID = 200
};

static PRInt16 nextMenuID = kBaseMenuID;
static PRInt16 nextSubMenuID = kBaseSubMenuID;

Boolean MenuFilter(long menuSelection)
{
    if (theActiveWindow != NULL) {
        UInt16 menuID = (menuSelection >> 16);
        if ((menuID >= kBaseMenuID && menuID < nextMenuID) || (menuID >= kBaseSubMenuID && menuID < nextSubMenuID)) {
            EventRecord menuEvent;
            ::OSEventAvail(0, &menuEvent);
            menuEvent.what = nsPluginEventType_MenuCommandEvent;
            menuEvent.message = menuSelection;

            WindowRef window = theActiveWindow->mWindow;
            nsPluginEvent pluginEvent = { &menuEvent, window };
            PRBool handled = PR_FALSE;
            theActiveWindow->mHandler->HandleEvent(&pluginEvent, &handled);
            
            return handled;
        }
    }
    return false;
}
