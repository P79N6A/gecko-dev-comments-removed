










































#include "nsMacMessagePump.h"

#include "nsIEventSink.h"
#include "nsIRollupListener.h"
#include "nsIWidget.h"
#include "nsPIWidgetMac.h"

#include "nsGfxUtils.h"
#include "nsGUIEvent.h"
#include "nsMacTSMMessagePump.h"
#include "nsToolkit.h"

#include <Carbon/Carbon.h>

#ifndef botRight
#define botRight(r) (((Point *) &(r))[1])
#endif

const short kMinWindowWidth = 125;
const short kMinWindowHeight = 150;

extern nsIRollupListener * gRollupListener;
extern nsIWidget         * gRollupWidget;

nsMacMessagePump::nsMacMessagePump(nsToolkit *aToolkit)
: mToolkit(aToolkit)
, mMouseClickEventHandler(NULL)
, mWNETransitionEventHandler(NULL)
{
  NS_ASSERTION(mToolkit, "No toolkit");
  
  
  
  
  
  const EventTypeSpec kWNETransitionEventList[] = {
    { kEventClassMouse,       kEventMouseDown },
    { kEventClassMouse,       kEventMouseUp },
    { kEventClassMouse,       kEventMouseMoved },
    { kEventClassMouse,       kEventMouseDragged },
    { kEventClassAppleEvent,  kEventAppleEvent },
    { kEventClassControl,     kEventControlTrack },
  };

  static EventHandlerUPP sWNETransitionEventHandlerUPP;
  if (!sWNETransitionEventHandlerUPP)
    sWNETransitionEventHandlerUPP =
                               ::NewEventHandlerUPP(WNETransitionEventHandler);

  OSStatus err =
   ::InstallApplicationEventHandler(sWNETransitionEventHandlerUPP,
                                    GetEventTypeCount(kWNETransitionEventList),
                                    kWNETransitionEventList,
                                    NS_STATIC_CAST(void*, this),
                                    &mWNETransitionEventHandler);

  NS_ASSERTION(err == noErr, "Could not install WNETransitionEventHandler");

  
  
  
  
  const EventTypeSpec kMouseClickEventList[] = {
    { kEventClassMouse, kEventMouseDown },
    { kEventClassMouse, kEventMouseUp },
  };

  static EventHandlerUPP sMouseClickEventHandlerUPP;
  if (!sMouseClickEventHandlerUPP)
    sMouseClickEventHandlerUPP = ::NewEventHandlerUPP(MouseClickEventHandler);

  err =
   ::InstallApplicationEventHandler(sMouseClickEventHandlerUPP,
                                    GetEventTypeCount(kMouseClickEventList),
                                    kMouseClickEventList,
                                    NS_STATIC_CAST(void*, this),
                                    &mMouseClickEventHandler);
  NS_ASSERTION(err == noErr, "Could not install MouseClickEventHandler");

  
  
  
  nsMacTSMMessagePump* tsmMessagePump = nsMacTSMMessagePump::GetSingleton();
  NS_ASSERTION(tsmMessagePump, "Unable to create TSM Message Pump");
}

nsMacMessagePump::~nsMacMessagePump()
{
  if (mMouseClickEventHandler)
    ::RemoveEventHandler(mMouseClickEventHandler);

  if (mWNETransitionEventHandler)
    ::RemoveEventHandler(mWNETransitionEventHandler);

  nsMacTSMMessagePump::Shutdown();
}






PRBool nsMacMessagePump::DispatchEvent(EventRecord *anEvent)
{
  PRBool handled = PR_FALSE;

  switch(anEvent->what) {
    
    
    

    case mouseDown:
      handled = DoMouseDown(*anEvent);
      break;

    case mouseUp:
      handled = DoMouseUp(*anEvent);
      break;

    case osEvt: {
      unsigned char eventType = ((anEvent->message >> 24) & 0x00ff);
      if (eventType == mouseMovedMessage)
        handled = DoMouseMove(*anEvent);
      break;
    }
      
    case kHighLevelEvent:
      ::AEProcessAppleEvent(anEvent);
      handled = PR_TRUE;
      break;
  }

  return handled;
}






PRBool nsMacMessagePump::DoMouseDown(EventRecord &anEvent)
{
  WindowPtr     whichWindow;
  WindowPartCode        partCode;
  PRBool  handled = PR_FALSE;
  
  partCode = ::FindWindow(anEvent.where, &whichWindow);
  
  switch (partCode)
  {
      case inNoWindow:
        break;

      case inCollapseBox:   
      case inSysWindow:
        if ( gRollupListener && gRollupWidget )
          gRollupListener->Rollup();
        break;

      case inMenuBar:
      {
        
        
        
        if ( gRollupListener && gRollupWidget )
        {
          gRollupListener->Rollup();
        }
        else
        {
          ::MenuSelect(anEvent.where);
          handled = PR_TRUE;
        }
        
        break;
      }

      case inContent:
      {
        nsGraphicsUtils::SafeSetPortWindowPort(whichWindow);
        if ( IsWindowHilited(whichWindow) || (gRollupListener && gRollupWidget) )
          handled = DispatchOSEventToRaptor(anEvent, whichWindow);
        else {
          nsCOMPtr<nsIWidget> topWidget;
          nsToolkit::GetTopWidget ( whichWindow, getter_AddRefs(topWidget) );
          nsCOMPtr<nsPIWidgetMac> macWindow ( do_QueryInterface(topWidget) );
          if ( macWindow ) {
            
            
            
            
            Boolean initiateDragFromBGWindow = ::WaitMouseMoved(anEvent.where);
            if ( initiateDragFromBGWindow ) {
              nsCOMPtr<nsIEventSink> sink ( do_QueryInterface(topWidget) );
              if ( sink ) {
                
                
                PRBool handled = PR_FALSE;
                sink->DispatchEvent ( &anEvent, &handled );
                
                EventRecord updateEvent = anEvent;
                updateEvent.what = updateEvt;
                updateEvent.message = NS_REINTERPRET_CAST(UInt32, whichWindow);
                sink->DispatchEvent ( &updateEvent, &handled );
                
                sink->DragEvent ( NS_DRAGDROP_GESTURE, anEvent.where.h, anEvent.where.v, 0L, &handled );                
              }
            }
            else {
              PRBool enabled;
              if (NS_SUCCEEDED(topWidget->IsEnabled(&enabled)) && !enabled)
                ::SysBeep(1);
              else
                macWindow->ComeToFront();
            }
            handled = PR_TRUE;
          }
        }
        break;
      }

      case inDrag:
      {
        nsGraphicsUtils::SafeSetPortWindowPort(whichWindow);

        Point   oldTopLeft = {0, 0};
        ::LocalToGlobal(&oldTopLeft);
        
        
        if ( gRollupListener && gRollupWidget )
          gRollupListener->Rollup();

        Rect screenRect;
        ::GetRegionBounds(::GetGrayRgn(), &screenRect);
        ::DragWindow(whichWindow, anEvent.where, &screenRect);

        Point   newTopLeft = {0, 0};
        ::LocalToGlobal(&newTopLeft);

        
        if (!(anEvent.modifiers & cmdKey))
        {
          nsCOMPtr<nsIWidget> topWidget;
          nsToolkit::GetTopWidget(whichWindow, getter_AddRefs(topWidget));
          
          nsCOMPtr<nsPIWidgetMac> macWindow ( do_QueryInterface(topWidget) );
          if ( macWindow )
            macWindow->ComeToFront();
        }
        
        
        anEvent.where.h += newTopLeft.h - oldTopLeft.h;
        anEvent.where.v += newTopLeft.v - oldTopLeft.v;
        
        handled = DispatchOSEventToRaptor(anEvent, whichWindow);
        break;
      }

      case inGrow:
      {
        nsGraphicsUtils::SafeSetPortWindowPort(whichWindow);

        Rect sizeLimit;
        sizeLimit.top = kMinWindowHeight;
        sizeLimit.left = kMinWindowWidth;
        sizeLimit.bottom = 0x7FFF;
        sizeLimit.right = 0x7FFF;

        Rect newSize;
        ::ResizeWindow(whichWindow, anEvent.where, &sizeLimit, &newSize);

        Point newPt = botRight(newSize);
        ::LocalToGlobal(&newPt);
        newPt.h -= 8, newPt.v -= 8;
        anEvent.where = newPt;  
        handled = DispatchOSEventToRaptor(anEvent, whichWindow);

        break;
      }

      case inGoAway:
      {
        nsGraphicsUtils::SafeSetPortWindowPort(whichWindow);
        if (::TrackGoAway(whichWindow, anEvent.where)) {
          handled = DispatchOSEventToRaptor(anEvent, whichWindow);
        }
        break;
      }

      case inZoomIn:
      case inZoomOut:
        if (::TrackBox(whichWindow, anEvent.where, partCode))
        {
          if (partCode == inZoomOut)
          {
            nsCOMPtr<nsIWidget> topWidget;
            nsToolkit::GetTopWidget ( whichWindow, getter_AddRefs(topWidget) );
            nsCOMPtr<nsPIWidgetMac> macWindow ( do_QueryInterface(topWidget) );
            if ( macWindow )
              macWindow->CalculateAndSetZoomedSize();
          }
          
          
          
          
          handled = DispatchOSEventToRaptor(anEvent, whichWindow);
        }
        break;

      case inToolbarButton:           
        nsGraphicsUtils::SafeSetPortWindowPort(whichWindow);
        handled = DispatchOSEventToRaptor(anEvent, whichWindow);
        break;

  }

  return handled;
}






PRBool nsMacMessagePump::DoMouseUp(EventRecord &anEvent)
{
    WindowPtr     whichWindow;
    PRInt16       partCode;

  partCode = ::FindWindow(anEvent.where, &whichWindow);
  if (whichWindow == nil)
  {
    
    
    
    
    whichWindow = ::FrontWindow();
  }
  
  PRBool handled = DispatchOSEventToRaptor(anEvent, whichWindow);
  
  if (partCode == inDrag)
    handled = PR_TRUE;
  return handled;
}






PRBool nsMacMessagePump::DoMouseMove(EventRecord &anEvent)
{
  
  WindowPtr     whichWindow;
  PRInt16       partCode;
  PRBool        handled = PR_FALSE;
  
  partCode = ::FindWindow(anEvent.where, &whichWindow);
  if (whichWindow == nil)
    whichWindow = ::FrontWindow();

  


  if (whichWindow == nil || !::IsWindowCollapsed(whichWindow))
    handled = DispatchOSEventToRaptor(anEvent, whichWindow);
  return handled;
}







PRBool  nsMacMessagePump::DispatchOSEventToRaptor(
                          EventRecord   &anEvent,
                          WindowPtr     aWindow)
{
  PRBool handled = PR_FALSE;
  nsCOMPtr<nsIEventSink> sink;
  nsToolkit::GetWindowEventSink ( aWindow, getter_AddRefs(sink) );
  if ( sink )
    sink->DispatchEvent ( &anEvent, &handled );
  return handled;
}

pascal OSStatus
nsMacMessagePump::MouseClickEventHandler(EventHandlerCallRef aHandlerCallRef,
                                         EventRef            aEvent,
                                         void*               aUserData)
{
  EventMouseButton button;
  OSErr err = ::GetEventParameter(aEvent, kEventParamMouseButton,
                                  typeMouseButton, NULL,
                                  sizeof(EventMouseButton), NULL, &button);

  
  if (err != noErr || button != kEventMouseButtonTertiary)
    return eventNotHandledErr;

  EventRecord eventRecord;
  if (!::ConvertEventRefToEventRecord(aEvent, &eventRecord)) {
    
    
    
    
    
    UInt32 kind = ::GetEventKind(aEvent);
    eventRecord.what = (kind == kEventMouseDown) ? mouseDown : mouseUp;
  }

  
  
  
  eventRecord.message = NS_STATIC_CAST(UInt32, button);

  
  nsMacMessagePump* self = NS_STATIC_CAST(nsMacMessagePump*, aUserData);
  PRBool handled = self->DispatchEvent(&eventRecord);

  if (handled)
    return noErr;

  return eventNotHandledErr;
}









pascal OSStatus
nsMacMessagePump::WNETransitionEventHandler(EventHandlerCallRef aHandlerCallRef,
                                            EventRef            aEvent,
                                            void*               aUserData)
{
  nsMacMessagePump* self = NS_STATIC_CAST(nsMacMessagePump*, aUserData);

  EventRecord eventRecord;
  ::ConvertEventRefToEventRecord(aEvent, &eventRecord);

  PRBool handled = self->DispatchEvent(&eventRecord);

  if (!handled)
    return eventNotHandledErr;

  return noErr;
}
