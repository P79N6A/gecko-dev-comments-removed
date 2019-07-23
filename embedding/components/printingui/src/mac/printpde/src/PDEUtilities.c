






















































#include <Carbon/Carbon.h>
#include <Print/PMPrintingDialogExtensions.h>

#include "PDECore.h"
#include "PDECustom.h"
#include "PDEUtilities.h"


static OSStatus MyHandleHelpEvent (EventHandlerCallRef, EventRef, void *userData);








extern void MyDebugMessage (char *msg, SInt32 value)
{
    char *debug = getenv ("PDEDebug");
    if (debug != NULL)
    {
        fprintf (stdout, "%s (%d)\n", msg, (int) value);
        fflush (stdout);
    }
}







extern CFTypeRef MyCFAssign(CFTypeRef srcRef, CFTypeRef dstRef)
{
    if (srcRef)
        CFRetain(srcRef);
    if (dstRef)
        CFRelease(dstRef);
    dstRef = srcRef;
    return dstRef;
}







static CFBundleRef _MyGetBundle (Boolean stillNeeded)
{
    static CFBundleRef sBundle = NULL;
    
    if (stillNeeded)
    {
        if (sBundle == NULL)
        {
            sBundle = CFBundleGetBundleWithIdentifier (kMyBundleIdentifier);
            CFRetain (sBundle);
        }
    }
    else
    {
        if (sBundle != NULL)
        {
            CFRelease (sBundle);
            sBundle = NULL;
        }
    }

    return sBundle;
}








extern CFBundleRef MyGetBundle()
{
    return _MyGetBundle (TRUE);
}








extern void MyFreeBundle()
{
    _MyGetBundle (FALSE);
}


#pragma mark -







extern CFStringRef MyGetTitle()
{
    return MyGetCustomTitle (TRUE);
}








extern void MyFreeTitle()
{
    MyGetCustomTitle (FALSE);
}


#pragma mark -







extern OSStatus MyGetTicket (
    PMPrintSession  session,
    CFStringRef     ticketID,
    PMTicketRef*    ticketPtr
)

{
    OSStatus result = noErr;
    CFTypeRef type = NULL;
    PMTicketRef ticket = NULL;
    
    *ticketPtr = NULL;

    result = PMSessionGetDataFromSession (session, ticketID, &type);

    if (result == noErr)
    {    
        if (CFNumberGetValue (
            (CFNumberRef) type, kCFNumberSInt32Type, (void*) &ticket))
        {
            *ticketPtr = ticket;
        }
        else {
            result = kPMInvalidValue;
        }
    }

    return result;
}








extern OSStatus MyEmbedControl (
    WindowRef nibWindow,
    ControlRef userPane,
    const ControlID *controlID,
    ControlRef* outControl
)

{
    ControlRef control = NULL;
    OSStatus result = noErr;

    *outControl = NULL;

    result = GetControlByID (nibWindow, controlID, &control);
    if (result == noErr)
    {
        SInt16 dh, dv;
        Rect nibFrame, controlFrame, paneFrame;

        (void) GetWindowBounds (nibWindow, kWindowContentRgn, &nibFrame);
        (void) GetControlBounds (userPane, &paneFrame);
        (void) GetControlBounds (control, &controlFrame);
        
        
        

        dh = ((paneFrame.right - paneFrame.left) - 
                (nibFrame.right - nibFrame.left))/2;

        if (dh < 0) dh = 0;

        dv = ((paneFrame.bottom - paneFrame.top) - 
                (nibFrame.bottom - nibFrame.top))/2;

        if (dv < 0) dv = 0;
                
        OffsetRect (
            &controlFrame, 
            paneFrame.left + dh, 
            paneFrame.top + dv
        );
 
        (void) SetControlBounds (control, &controlFrame);

        
        result = SetControlVisibility (control, TRUE, FALSE);

        if (result == noErr) 
        {
            result = EmbedControl (control, userPane);
            if (result == noErr)
            {
                
                *outControl = control;
            }
        }
    }

    return result;
}


#pragma mark -







extern void MyReleaseContext (MyContext context)
{
    if (context != NULL)
    {
        if (context->customContext != NULL) {
            MyReleaseCustomContext (context->customContext);
        }

        free (context);
    }
}


#pragma mark -







#define kMyNumberOfEventTypes   1

extern OSStatus MyInstallHelpEventHandler (
    WindowRef inWindow, 
    EventHandlerRef *outHandler,
    EventHandlerUPP *outHandlerUPP
)

{
    static const EventTypeSpec sEventTypes [kMyNumberOfEventTypes] =
    {
        { kEventClassCommand, kEventCommandProcess }
    };

    OSStatus result = noErr;
    EventHandlerRef handler = NULL;
    EventHandlerUPP handlerUPP = NewEventHandlerUPP (MyHandleHelpEvent);

    result = InstallWindowEventHandler (
        inWindow,
        handlerUPP,
        kMyNumberOfEventTypes,
        sEventTypes,
        NULL, 
        &handler
    );

    *outHandler = handler;
    *outHandlerUPP = handlerUPP;
    
    MyDebugMessage("InstallEventHandler", result);
    return result;
}








extern OSStatus MyRemoveHelpEventHandler (
    EventHandlerRef *helpHandlerP, 
    EventHandlerUPP *helpHandlerUPP
)

{
    OSStatus result = noErr;
    
    
    if (*helpHandlerP != NULL)
    {
        MyDebugMessage("Removing event handler", result);
        result = RemoveEventHandler (*helpHandlerP);
        *helpHandlerP = NULL;
    }

    if (*helpHandlerUPP != NULL)
    {
        DisposeEventHandlerUPP (*helpHandlerUPP);
        *helpHandlerUPP = NULL;
    }
    return result;
}








static OSStatus MyHandleHelpEvent
(
    EventHandlerCallRef call,
    EventRef event, 
    void *userData
)

{
    HICommand   commandStruct;
    OSStatus    result = eventNotHandledErr;

    GetEventParameter (
        event, kEventParamDirectObject,
        typeHICommand, NULL, sizeof(HICommand), 
        NULL, &commandStruct
    );

    if (commandStruct.commandID == 'help')
    {
        
        
    }

    return result;
}



