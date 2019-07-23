





































#include "nsMacWindow.h"
#include "nsMacEventHandler.h"
#include "nsToolkit.h"

#include "nsIServiceManager.h"    
#include "nsWidgetsCID.h"
#include "nsIDragHelperService.h"
#include "nsIScreen.h"
#include "nsIScreenManager.h"
#include "nsGUIEvent.h"
#include "nsCarbonHelpers.h"
#include "nsGfxUtils.h"
#include "nsMacResources.h"
#include "nsIRollupListener.h"
#include "nsCRT.h"
#include "nsWidgetSupport.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

#if MAC_OS_X_VERSION_MAX_ALLOWED <= MAC_OS_X_VERSION_10_4






#define kEventParamMouseWheelSmoothVerticalDelta   'saxy'
#define kEventParamMouseWheelSmoothHorizontalDelta 'saxx'
#define kEventMouseScroll 11
#endif

typedef OSStatus (*TransitionWindowWithOptions_type) (WindowRef,
                                             WindowTransitionEffect,
                                             WindowTransitionAction,
                                             const HIRect*,
                                             Boolean,
                                             TransitionWindowOptions*);

static const char sScreenManagerContractID[] = "@mozilla.org/gfx/screenmanager;1";


#ifndef topLeft
  #define topLeft(r)  (((Point *) &(r))[0])
#endif
#ifndef botRight
  #define botRight(r) (((Point *) &(r))[1])
#endif


extern nsIRollupListener * gRollupListener;
extern nsIWidget         * gRollupWidget;


#define kWindowPositionSlop 20



const short kWindowTitleBarHeight = 22;
const short kWindowMarginWidth = 0;
const short kDialogTitleBarHeight = 22;
const short kDialogMarginWidth = 0;

#if 0


static void PrintRgn(const char* inLabel, RgnHandle inRgn)
{
  Rect regionBounds;
  GetRegionBounds(inRgn, &regionBounds);
  printf("%s left %d, top %d, right %d, bottom %d\n", inLabel,
    regionBounds.left, regionBounds.top, regionBounds.right, regionBounds.bottom);
}

static void PrintPortState(WindowPtr inWindow, const char* label)
{
  CGrafPtr currentPort = CGrafPtr(GetQDGlobalsThePort());
  Rect bounds;
  GetPortBounds(currentPort, &bounds);
  printf("%s: Current port: %p, top, left = %d, %d\n", label, currentPort, bounds.top, bounds.left);

  StRegionFromPool savedClip;
  ::GetClip(savedClip);
  PrintRgn("  clip:", savedClip);

  StRegionFromPool updateRgn;
  ::GetWindowUpdateRegion(inWindow, updateRgn);
  Rect windowBounds;
  ::GetWindowBounds(inWindow, kWindowContentRgn, &windowBounds);
  ::OffsetRgn(updateRgn, -windowBounds.left, -windowBounds.top);

  PrintRgn("  update:", updateRgn);
}

#endif

#pragma mark -

pascal OSErr
nsMacWindow::DragTrackingHandler ( DragTrackingMessage theMessage, WindowPtr theWindow, 
                    void *handlerRefCon, DragReference theDrag)
{
  static nsCOMPtr<nsIDragHelperService> sDragHelper;

  nsCOMPtr<nsIEventSink> windowEventSink;
  nsToolkit::GetWindowEventSink(theWindow, getter_AddRefs(windowEventSink));
  if ( !theWindow || !windowEventSink )
    return dragNotAcceptedErr;
    
  switch ( theMessage ) {
  
    case kDragTrackingEnterHandler:
      break;
      
    case kDragTrackingEnterWindow:
    {
      sDragHelper = do_GetService ( "@mozilla.org/widget/draghelperservice;1" );
      NS_ASSERTION ( sDragHelper, "Couldn't get a drag service, we're in biiig trouble" );
      if ( sDragHelper )
        sDragHelper->Enter ( theDrag, windowEventSink );
      break;
    }
    
    case kDragTrackingInWindow:
    {
      if ( sDragHelper ) {
        PRBool dropAllowed = PR_FALSE;
        sDragHelper->Tracking ( theDrag, windowEventSink, &dropAllowed );
      }
      break;
    }
    
    case kDragTrackingLeaveWindow:
    {
      if ( sDragHelper ) {
        sDragHelper->Leave ( theDrag, windowEventSink );
        sDragHelper = nsnull;      
      }
      break;
    }
    
  } 

  return noErr;
  
} 


pascal OSErr
nsMacWindow::DragReceiveHandler (WindowPtr theWindow, void *handlerRefCon,
                  DragReference theDragRef)
{
  nsCOMPtr<nsIEventSink> windowEventSink;
  nsToolkit::GetWindowEventSink(theWindow, getter_AddRefs(windowEventSink));
  if ( !theWindow || !windowEventSink )
    return dragNotAcceptedErr;
  
  
  
  OSErr result = noErr;
  nsCOMPtr<nsIDragHelperService> helper ( do_GetService("@mozilla.org/widget/draghelperservice;1") );
  if ( helper ) {
    PRBool dragAccepted = PR_FALSE;
    helper->Drop ( theDragRef, windowEventSink, &dragAccepted );
    if ( !dragAccepted )
      result = dragNotAcceptedErr;
  }
    
  return result;
  
} 


NS_IMPL_ISUPPORTS_INHERITED4(nsMacWindow, Inherited, nsIEventSink, nsPIWidgetMac, nsPIEventSinkStandalone, 
                                          nsIMacTextInputEventSink)







nsMacWindow::nsMacWindow() : Inherited()
  , mWindowMadeHere(PR_FALSE)
  , mIsSheet(PR_FALSE)
  , mAcceptsActivation(PR_TRUE)
  , mIsActive(PR_FALSE)
  , mZoomOnShow(PR_FALSE)
  , mZooming(PR_FALSE)
  , mResizeIsFromUs(PR_FALSE)
  , mShown(PR_FALSE)
  , mSheetNeedsShow(PR_FALSE)
  , mInPixelMouseScroll(PR_FALSE)
  , mMacEventHandler(nsnull)
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
  , mNeedsResize(PR_FALSE)
#endif
{
  WIDGET_SET_CLASSNAME("nsMacWindow");  

  
  mDragTrackingHandlerUPP = NewDragTrackingHandlerUPP(DragTrackingHandler);
  mDragReceiveHandlerUPP = NewDragReceiveHandlerUPP(DragReceiveHandler);
  mBoundsOffset.v = kWindowTitleBarHeight; 
  mBoundsOffset.h = kWindowMarginWidth;
}







nsMacWindow::~nsMacWindow()
{
  if ( mWindowPtr && mWindowMadeHere ) {
    
    if ( mDragTrackingHandlerUPP ) {
      ::RemoveTrackingHandler ( mDragTrackingHandlerUPP, mWindowPtr );
      ::DisposeDragTrackingHandlerUPP ( mDragTrackingHandlerUPP );
     }
    if ( mDragReceiveHandlerUPP ) {
      ::RemoveReceiveHandler ( mDragReceiveHandlerUPP, mWindowPtr );
      ::DisposeDragReceiveHandlerUPP ( mDragReceiveHandlerUPP );
    }

    
    CGrafPtr    curPort;
    CGrafPtr    windowPort = ::GetWindowPort(mWindowPtr);
    ::GetPort((GrafPtr*)&curPort);
    PRBool      mustResetPort = (curPort == windowPort);
    
    
    ::DisposeWindow(mWindowPtr);
    mWindowPtr = nsnull;
    
    if (mustResetPort)
      nsGraphicsUtils::SetPortToKnownGoodPort();
  }
  else if ( mWindowPtr && !mWindowMadeHere ) {
    (void)::RemoveWindowProperty(mWindowPtr, kTopLevelWidgetPropertyCreator,
        kTopLevelWidgetRefPropertyTag);
  }
}







nsresult nsMacWindow::StandardCreate(nsIWidget *aParent,
                        const nsRect &aRect,
                        EVENT_CALLBACK aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell *aAppShell,
                        nsIToolkit *aToolkit,
                        nsWidgetInitData *aInitData,
                        nsNativeWidget aNativeParent)
{
  short bottomPinDelta = 0;     
  nsCOMPtr<nsIToolkit> theToolkit = aToolkit;

  NS_ASSERTION(!aInitData || aInitData->mWindowType != eWindowType_popup ||
               !aParent, "Popups should not be hooked into nsIWidget hierarchy");

  nsMacEventDispatchHandler* eventDispatchHandler = nsnull;

  if (aInitData && aInitData->mWindowType == eWindowType_popup &&
      aNativeParent) {
    
    
    
    

    
    
    
    nsIWidget* widget = NS_STATIC_CAST(nsIWidget*, aNativeParent);
    nsIWidget* topWidget = nsnull;

    while (widget && (widget = widget->GetParent()))
      topWidget = widget;

    if (topWidget) {
      nsCOMPtr<nsPIWidgetMac> parentMacWindow = do_QueryInterface(topWidget);

      if (parentMacWindow)
        parentMacWindow->GetEventDispatchHandler(&eventDispatchHandler);
    }
  }

  mMacEventHandler.reset(new nsMacEventHandler(this, eventDispatchHandler));

  
  if (!aNativeParent || (aInitData && aInitData->mWindowType == eWindowType_popup))
  {
    PRBool allOrDefault;

    if (aInitData) {
      allOrDefault = aInitData->mBorderStyle == eBorderStyle_all ||
                     aInitData->mBorderStyle == eBorderStyle_default;
      mWindowType = aInitData->mWindowType;
      
      if (aInitData->mWindowType == eWindowType_toplevel &&
          (aInitData->mBorderStyle == eBorderStyle_none ||
           !allOrDefault &&
           !(aInitData->mBorderStyle & eBorderStyle_title)))
        mWindowType = eWindowType_dialog;
    }
    else
    {
      allOrDefault = PR_TRUE;
      mWindowType = eWindowType_toplevel;
    }

    static const WindowAttributes kWindowResizableAttributes =
      kWindowResizableAttribute | kWindowLiveResizeAttribute;

    WindowClass windowClass;
    WindowAttributes attributes = kWindowNoAttributes;
    short hOffset = 0, vOffset = 0;

    switch (mWindowType)
    {
      case eWindowType_popup:
        
        
        
        mOffsetParent = aParent;
        if( aParent )
          theToolkit = getter_AddRefs(aParent->GetToolkit());

        mAcceptsActivation = PR_FALSE;

        
        
        
        windowClass = 18; 
        break;

      case eWindowType_child:
        windowClass = kPlainWindowClass;
        break;

      case eWindowType_dialog:
        mIsTopWidgetWindow = PR_TRUE;
        if (aInitData)
        {
          
          switch (aInitData->mBorderStyle)
          {
            case eBorderStyle_none:
              windowClass = kModalWindowClass;
              break;

            case eBorderStyle_default:
              windowClass = kDocumentWindowClass;
              break;

            case eBorderStyle_all:
              windowClass = kDocumentWindowClass;
              attributes = kWindowCollapseBoxAttribute |
                           kWindowResizableAttributes;
              break;

            default:
                windowClass = kDocumentWindowClass;

                
                switch(aInitData->mBorderStyle & (eBorderStyle_resizeh | eBorderStyle_title))
                {
                  
                  case eBorderStyle_title:
                    attributes = kWindowCollapseBoxAttribute;
                    break;

                  case eBorderStyle_resizeh:
                  case (eBorderStyle_title | eBorderStyle_resizeh):
                    attributes =
                      kWindowCollapseBoxAttribute | kWindowResizableAttributes;
                    break;

                  case eBorderStyle_none: 
                    
                    
                    
                    
                    
                    windowClass = 18; 
                    break;

                  default: 
                    NS_WARNING("Unhandled combination of window flags");
                    break;
                }
              }
          }
        else
        {
          windowClass = kMovableModalWindowClass;
          attributes = kWindowCollapseBoxAttribute;
        }

        hOffset = kDialogMarginWidth;
        vOffset = kDialogTitleBarHeight;
        break;

      case eWindowType_sheet:
        mIsTopWidgetWindow = PR_TRUE;
        if (aInitData)
        {
          nsWindowType parentType;
          aParent->GetWindowType(parentType);
          if (parentType != eWindowType_invisible)
          {
            
            mIsSheet = PR_TRUE;
            windowClass = kSheetWindowClass;
            if (aInitData->mBorderStyle & eBorderStyle_resizeh)
            {
              attributes = kWindowResizableAttributes;
            }
          }
          else
          {
            windowClass = kDocumentWindowClass;
            attributes = kWindowCollapseBoxAttribute;
          }
        }
        else
        {
          windowClass = kMovableModalWindowClass;
          attributes = kWindowCollapseBoxAttribute;
        }

        hOffset = kDialogMarginWidth;
        vOffset = kDialogTitleBarHeight;
        break;

      case eWindowType_toplevel:
        mIsTopWidgetWindow = PR_TRUE;
        windowClass = kDocumentWindowClass;
        attributes =
          kWindowCollapseBoxAttribute | kWindowToolbarButtonAttribute;

        if (allOrDefault || aInitData->mBorderStyle & eBorderStyle_close)
          attributes |= kWindowCloseBoxAttribute;

        if (allOrDefault || aInitData->mBorderStyle & eBorderStyle_resizeh)
          attributes |= kWindowFullZoomAttribute | kWindowResizableAttributes;

        hOffset = kWindowMarginWidth;
        vOffset = kWindowTitleBarHeight;
        break;

      case eWindowType_invisible:
        
        
        
        windowClass = kPlainWindowClass;

        
        
        
        if (nsToolkit::OSXVersion() >= MAC_OS_X_VERSION_10_2_HEX)
        {
          attributes = (1L << 15); 
        }
        break;

      default:
        NS_ERROR("Unhandled window type!");

        return NS_ERROR_FAILURE;
    }

    Rect wRect;
    nsRectToMacRect(aRect, wRect);

    if (eWindowType_popup != mWindowType)
      ::OffsetRect(&wRect, hOffset, vOffset + ::GetMBarHeight());
    else
      ::OffsetRect(&wRect, hOffset, vOffset);

    nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
    if (screenmgr) {
      nsCOMPtr<nsIScreen> screen;
      
      screenmgr->ScreenForRect(wRect.left, wRect.top,
                                 wRect.right - wRect.left, wRect.bottom - wRect.top,
                                 getter_AddRefs(screen));
      if (screen) {
        PRInt32 left, top, width, height;
        screen->GetAvailRect(&left, &top, &width, &height);
        if (wRect.bottom > top+height) {
          bottomPinDelta = wRect.bottom - (top+height);
          wRect.bottom -= bottomPinDelta;
        }
      }
    }

    ::CreateNewWindow(windowClass, attributes, &wRect, &mWindowPtr);

    mWindowMadeHere = PR_TRUE;

    
    
    Rect structure;
    ::GetWindowBounds(mWindowPtr, kWindowStructureRgn, &structure);
    mBoundsOffset.v = wRect.top - structure.top;
    mBoundsOffset.h = wRect.left - structure.left;
  }
  else
  {
    mWindowPtr = (WindowPtr)aNativeParent;
    mWindowMadeHere = PR_FALSE;
    mVisible = PR_TRUE;
  }

  if (mWindowPtr == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  
  
  
  
  nsIWidget* temp = NS_STATIC_CAST(nsIWidget*, this);
  OSStatus err = ::SetWindowProperty ( mWindowPtr,
                          kTopLevelWidgetPropertyCreator, kTopLevelWidgetRefPropertyTag,
                          sizeof(nsIWidget*), &temp );
  NS_ASSERTION ( err == noErr, "couldn't set a property on the window, event handling will fail" );
  if ( err != noErr )
    return NS_ERROR_FAILURE;

  
  
  nsRect bounds(0, 0, aRect.width, aRect.height - bottomPinDelta);

  
  if (!aInitData || aInitData->mWindowType != eWindowType_sheet)
    aParent = nil;

  
  
  Inherited::StandardCreate(aParent, bounds, aHandleEventFunction, aContext, aAppShell, theToolkit, aInitData);

  
  
  
  
  if ( mWindowMadeHere ) {
    if ( mWindowType == eWindowType_popup ) {
      
      
      
      ::SetWindowGroup(mWindowPtr, ::GetWindowGroupOfClass(kHelpWindowClass));
      ::SetWindowActivationScope(mWindowPtr, kWindowActivationScopeNone);
    }

    if ( mWindowType != eWindowType_invisible &&
         mWindowType != eWindowType_plugin &&
         mWindowType != eWindowType_java) {
      const EventTypeSpec kScrollEventList[] = {
        { kEventClassMouse, kEventMouseWheelMoved },
#if 0
        
        { kEventClassMouse, kEventMouseScroll },
#endif
      };

      static EventHandlerUPP sScrollEventHandlerUPP;
      if (!sScrollEventHandlerUPP)
        sScrollEventHandlerUPP = ::NewEventHandlerUPP(ScrollEventHandler);

      err = ::InstallWindowEventHandler(mWindowPtr,
                                        sScrollEventHandlerUPP,
                                        GetEventTypeCount(kScrollEventList),
                                        kScrollEventList,
                                        (void*)this,
                                        NULL);
      NS_ASSERTION(err == noErr, "Couldn't install scroll event handler");
    }

    
    const EventTypeSpec kWindowEventList[] = {
      
      { kEventClassWindow, kEventWindowBoundsChanged },
      
      { kEventClassWindow, kEventWindowCollapsing },
      
      { kEventClassWindow, kEventWindowExpanded },
      
      { kEventClassWindow, kEventWindowConstrain },
      
      { kEventClassWindow, kEventWindowUpdate },
      
      { kEventClassWindow, kEventWindowActivated },
      { kEventClassWindow, kEventWindowDeactivated },
      
      { kEventClassWindow, kEventWindowTransitionCompleted },
    };

    static EventHandlerUPP sWindowEventHandlerUPP;
    if (!sWindowEventHandlerUPP)
      sWindowEventHandlerUPP = ::NewEventHandlerUPP(WindowEventHandler);

    err = ::InstallWindowEventHandler(mWindowPtr,
                                      sWindowEventHandlerUPP,
                                      GetEventTypeCount(kWindowEventList),
                                      kWindowEventList,
                                      (void*)this,
                                      NULL);
    NS_ASSERTION(err == noErr, "Couldn't install window event handler");

    
    const EventTypeSpec kKeyEventList[] = {
      { kEventClassKeyboard, kEventRawKeyDown },
      { kEventClassKeyboard, kEventRawKeyUp },
      { kEventClassKeyboard, kEventRawKeyModifiersChanged },
    };

    static EventHandlerUPP sKeyEventHandlerUPP;
    if (!sKeyEventHandlerUPP)
      sKeyEventHandlerUPP = ::NewEventHandlerUPP(KeyEventHandler);

    err = ::InstallWindowEventHandler(mWindowPtr,
                                      sKeyEventHandlerUPP,
                                      GetEventTypeCount(kKeyEventList),
                                      kKeyEventList,
                                      NS_STATIC_CAST(void*, this),
                                      NULL);
    NS_ASSERTION(err == noErr, "Couldn't install key event handler");

    
    if ( mDragTrackingHandlerUPP ) {
      err = ::InstallTrackingHandler ( mDragTrackingHandlerUPP, mWindowPtr, nsnull );
      NS_ASSERTION ( err == noErr, "can't install drag tracking handler");
    }
    if ( mDragReceiveHandlerUPP ) {
      err = ::InstallReceiveHandler ( mDragReceiveHandlerUPP, mWindowPtr, nsnull );
      NS_ASSERTION ( err == noErr, "can't install drag receive handler");
    }

    if (mWindowType == eWindowType_popup) {
      
      
      
      
      const float kPopupWindowAlpha = 0.95;
      ::SetWindowAlpha(mWindowPtr, kPopupWindowAlpha);
    }

  } 

  nsGraphicsUtils::SafeSetPortWindowPort(mWindowPtr);

  return NS_OK;
}



pascal OSStatus
nsMacWindow::ScrollEventHandler(EventHandlerCallRef aHandlerCallRef,
                                EventRef            aEvent,
                                void*               aUserData)
{
  OSStatus retVal = eventNotHandledErr;

  Point mouseLoc;
  UInt32 modifiers = 0;
  if (::GetEventParameter(aEvent, kEventParamMouseLocation,
                          typeQDPoint, NULL,
                          sizeof(Point), NULL, &mouseLoc) != noErr ||
      ::GetEventParameter(aEvent, kEventParamKeyModifiers,
                          typeUInt32, NULL,
                          sizeof(UInt32), NULL, &modifiers) != noErr) {
    
    return retVal;
  }

  SInt32 deltaY = 0, deltaX = 0;
  PRBool isPixels = PR_FALSE;

  nsMacWindow* self = NS_REINTERPRET_CAST(nsMacWindow*, aUserData);

  EventKind kind = ::GetEventKind(aEvent);

  switch (kind) {
    case kEventMouseWheelMoved: {
      
      

      if (self->mInPixelMouseScroll) {
        
        
        return noErr; 
      }

      EventMouseWheelAxis axis = kEventMouseWheelAxisY;
      SInt32 delta = 0;
      if (::GetEventParameter(aEvent, kEventParamMouseWheelAxis,
                              typeMouseWheelAxis, NULL,
                              sizeof(EventMouseWheelAxis), NULL,
                              &axis) != noErr ||
          ::GetEventParameter(aEvent, kEventParamMouseWheelDelta,
                              typeLongInteger, NULL,
                              sizeof(SInt32), NULL, &delta) != noErr) {
        
        return retVal;
      }

      if (axis == kEventMouseWheelAxisY)
        deltaY = delta;
      else
        deltaX = delta;

      break;
    }

    case kEventMouseScroll: {
      
      
      
      isPixels = PR_TRUE;
      OSErr errY, errX;
      errY = ::GetEventParameter(aEvent,
                                 kEventParamMouseWheelSmoothVerticalDelta,
                                 typeSInt32, NULL,
                                 sizeof(SInt32), NULL, &deltaY);
      errX = ::GetEventParameter(aEvent,
                                 kEventParamMouseWheelSmoothHorizontalDelta,
                                 typeSInt32, NULL,
                                 sizeof(SInt32), NULL, &deltaX);
      if (errY != noErr && errX != noErr) {
        
        return retVal;
      }
      if ((errY != noErr && errY != eventParameterNotFoundErr) ||
          (errX != noErr && errX != eventParameterNotFoundErr)) {
        
        
        return retVal;
      }
      break;
    }

    default: {
      
      return retVal;
      break;
    }
  }

  {
    
    
    StPortSetter portSetter(self->mWindowPtr);
    StOriginSetter originSetter(self->mWindowPtr);
    ::GlobalToLocal(&mouseLoc);
    self->mMacEventHandler->Scroll(deltaY, deltaX, isPixels, mouseLoc,
                                   self, modifiers);
    retVal = noErr;
  }

  if (kind == kEventMouseScroll && (deltaX != 0 || deltaY != 0)) {
    
    
    
    
    
    
    PRBool lastInPixelMouseScroll = self->mInPixelMouseScroll;
    self->mInPixelMouseScroll = PR_TRUE;
    ::CallNextEventHandler(aHandlerCallRef, aEvent);
    self->mInPixelMouseScroll = lastInPixelMouseScroll;
  }

  return retVal;
} 


pascal OSStatus
nsMacWindow::WindowEventHandler ( EventHandlerCallRef inHandlerChain, EventRef inEvent, void* userData )
{
  OSStatus retVal = eventNotHandledErr;  
  nsMacWindow* self = NS_REINTERPRET_CAST(nsMacWindow*, userData);
  if (self) {
    UInt32 what = ::GetEventKind(inEvent);
    switch (what) {
    
      case kEventWindowBoundsChanged:
      {
        
        UInt32 attributes = 0;
        ::GetEventParameter ( inEvent, kEventParamAttributes, typeUInt32, NULL, sizeof(attributes), NULL, &attributes );
        if ( attributes & kWindowBoundsChangeSizeChanged ) {
          WindowRef myWind = NULL;
          ::GetEventParameter(inEvent, kEventParamDirectObject, typeWindowRef, NULL, sizeof(myWind), NULL, &myWind);
          Rect bounds;
          ::InvalWindowRect(myWind, ::GetWindowPortBounds(myWind, &bounds));
          
          
          NS_ENSURE_TRUE(self->mMacEventHandler.get(), eventNotHandledErr);
          if (!self->mResizeIsFromUs ) {
            self->mMacEventHandler->ResizeEvent(myWind);
            self->Update();
          }
          retVal = noErr;  
        }
        break;
      }
      
      case kEventWindowConstrain:
      {
        
        
        if ( self->mWindowType != eWindowType_invisible )
          retVal = ::CallNextEventHandler(inHandlerChain, inEvent);
        else
          retVal = noErr;  
        break;
      }      

      case kEventWindowUpdate:
      {
        self->Update();
        retVal = noErr; 
      }
      break;

      
      case kEventWindowCollapsing:
      {
        if ( gRollupListener && gRollupWidget )
          gRollupListener->Rollup();        
        self->mMacEventHandler->GetEventDispatchHandler()->DispatchGuiEvent(self, NS_DEACTIVATE);
        ::CallNextEventHandler(inHandlerChain, inEvent);
        retVal = noErr; 
      }
      break;

      
      
      case kEventWindowExpanded:
      {
        self->mMacEventHandler->GetEventDispatchHandler()->DispatchGuiEvent(self, NS_ACTIVATE);
        ::CallNextEventHandler(inHandlerChain, inEvent);
        retVal = noErr; 
      }
      break;

      case kEventWindowActivated:
      case kEventWindowDeactivated:
      {
        self->mMacEventHandler->HandleActivateEvent(inEvent);
        ::CallNextEventHandler(inHandlerChain, inEvent);
        retVal = noErr; 
      }
      break;

      case kEventWindowTransitionCompleted: {
        self->mDeathGripDuringTransition = nsnull;
        retVal = noErr;
        break;
      }

    } 
  }
  
  return retVal;
  
} 








NS_IMETHODIMP nsMacWindow::Create(nsNativeWidget aNativeParent,   
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData)
{
  return(StandardCreate(nsnull, aRect, aHandleEventFunction,
                          aContext, aAppShell, aToolkit, aInitData,
                            aNativeParent));
}







NS_IMETHODIMP nsMacWindow::Show(PRBool aState)
{
  
  nsIWidget *parentWidget = mParent;
  nsCOMPtr<nsPIWidgetMac> piParentWidget ( do_QueryInterface(parentWidget) );
  WindowRef parentWindowRef = (parentWidget) ?
    reinterpret_cast<WindowRef>(parentWidget->GetNativeData(NS_NATIVE_DISPLAY)) : nsnull;

  Inherited::Show(aState);
  
  
  
  
  if (aState && !mBounds.IsEmpty()) {
    if (mIsSheet) {
      if (parentWindowRef) {
        WindowPtr top = parentWindowRef;
        if (piParentWidget) {
          PRBool parentIsSheet = PR_FALSE;
          if (NS_SUCCEEDED(piParentWidget->GetIsSheet(&parentIsSheet)) &&
              parentIsSheet) {
            
            
            
            
            
            ::GetSheetWindowParent(parentWindowRef, &top);
            ::HideSheetWindow(parentWindowRef);
          }
        }

        nsMacWindow* sheetShown = nsnull;
        if (NS_SUCCEEDED(piParentWidget->GetChildSheet(PR_TRUE,
                                                       &sheetShown)) &&
            (!sheetShown || sheetShown == this)) {
          if (!sheetShown) {
            

            
            
            
            
            
            mShown = PR_TRUE;
            mSheetNeedsShow = PR_FALSE;

            ::ShowSheetWindow(mWindowPtr, top);
          }
          UpdateWindowMenubar(parentWindowRef, PR_FALSE);
          mMacEventHandler->GetEventDispatchHandler()->DispatchGuiEvent(this, NS_GOTFOCUS);
          mMacEventHandler->GetEventDispatchHandler()->DispatchGuiEvent(this, NS_ACTIVATE);
          ComeToFront();
        }
        else {
          
          
          
          mSheetNeedsShow = PR_TRUE;
        }
      }
    }
    else {
      if (mAcceptsActivation)
        ::ShowWindow(mWindowPtr);
      else {
        ::ShowHide(mWindowPtr, true);
        
        ::BringToFront(mWindowPtr);
      }
      ComeToFront();
      mShown = PR_TRUE;
    }

    if (mZoomOnShow) {
      SetSizeMode(nsSizeMode_Maximized);
      mZoomOnShow = PR_FALSE;
    }

    if (::IsWindowCollapsed(mWindowPtr))
      ::CollapseWindow(mWindowPtr, false);
  }
  else {
    
    
    
    
    if ( mWindowType == eWindowType_toplevel ) {
      if ( gRollupListener )
        gRollupListener->Rollup();
      NS_IF_RELEASE(gRollupListener);
      NS_IF_RELEASE(gRollupWidget);
    }

    
    if (mIsSheet) {
      if (mShown) {
        
        
        mShown = PR_FALSE;

        
        
        WindowPtr sheetParent = nsnull;
        ::GetSheetWindowParent(mWindowPtr, &sheetParent);

        ::HideSheetWindow(mWindowPtr);

        mMacEventHandler->GetEventDispatchHandler()->DispatchGuiEvent(this, NS_DEACTIVATE);

        WindowPtr top = GetWindowTop(parentWindowRef);
        nsMacWindow* siblingSheetToShow = nsnull;
        PRBool parentIsSheet = PR_FALSE;

        if (parentWindowRef && piParentWidget &&
            NS_SUCCEEDED(piParentWidget->GetChildSheet(PR_FALSE, 
                                                       &siblingSheetToShow)) &&
            siblingSheetToShow) {
          
          siblingSheetToShow->Show(PR_TRUE);
        }
        else if (parentWindowRef && piParentWidget &&
                 NS_SUCCEEDED(piParentWidget->GetIsSheet(&parentIsSheet)) &&
                 parentIsSheet) {
          
          
          
          ::ShowSheetWindow(parentWindowRef, sheetParent);
        }
        else {
          
          

          
          
          if (mAcceptsActivation)
            ::ShowWindow(top);
          else {
            ::ShowHide(top, true);
            
            ::BringToFront(top);
          }
        }
        ComeToFront();

        if (top == parentWindowRef)
          UpdateWindowMenubar(parentWindowRef, PR_TRUE);
      }
      else if (mSheetNeedsShow) {
        
        
        
        mSheetNeedsShow = PR_FALSE;
      }
    }
    else {
      if (mWindowPtr) {
#ifndef MOZ_SUNBIRD




        static TransitionWindowWithOptions_type transitionFunc;
        if (mWindowType == eWindowType_popup) {
          
          
          
          
          
          static PRBool sChecked;
          if (!sChecked) {
            sChecked = PR_TRUE;
#if MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_3
            transitionFunc = ::TransitionWindowWithOptions;
#else
            CFBundleRef carbonBundle =
             ::CFBundleGetBundleWithIdentifier(CFSTR("com.apple.Carbon"));
            if (carbonBundle) {
              transitionFunc = (TransitionWindowWithOptions_type)
               ::CFBundleGetFunctionPointerForName(carbonBundle,
                                         CFSTR("TransitionWindowWithOptions"));
            }
#endif
          }
        }
        
        
        
        
        
        if (mWindowType == eWindowType_popup && transitionFunc
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_4
            && nsToolkit::OSXVersion() >= MAC_OS_X_VERSION_10_4_HEX
#endif
           ) {
          mDeathGripDuringTransition = this;
          TransitionWindowOptions transitionOptions = { version  : 0,
                                                        duration : 0.2,
                                                        window   : nsnull,
                                                        userData : nsnull };
          transitionFunc(mWindowPtr, kWindowFadeTransitionEffect,
                         kWindowHideTransitionAction, nsnull,
                         PR_TRUE, &transitionOptions);
        }
        else
#endif
          ::HideWindow(mWindowPtr);
      }
      mShown = PR_FALSE;
    }
  }

  return NS_OK;
}










WindowPtr
nsMacWindow::GetWindowTop(WindowPtr baseWindowRef)
{
    if (!baseWindowRef) return(nsnull);





















    WindowPtr aSheet = ::GetFrontWindowOfClass(kSheetWindowClass, true);
    while(aSheet)
    {
        WindowRef   sheetParent;
        GetSheetWindowParent(aSheet, &sheetParent);
        if (sheetParent == baseWindowRef)
        {
            return(aSheet);
        }
        aSheet = GetNextWindowOfClass(aSheet, kSheetWindowClass, true);
    }
    return(baseWindowRef);
}


void
nsMacWindow::UpdateWindowMenubar(WindowPtr nativeWindowPtr, PRBool enableFlag)
{
    
    
    

    if (!nativeWindowPtr) return;

    nsCOMPtr<nsIWidget> windowWidget;
    nsToolkit::GetTopWidget ( nativeWindowPtr, getter_AddRefs(windowWidget));
    if (!windowWidget) return;

    nsCOMPtr<nsPIWidgetMac> parentWindow ( do_QueryInterface(windowWidget) );
    if (!parentWindow)  return;
    nsCOMPtr<nsIMenuBar> menubar;
    parentWindow->GetMenuBar(getter_AddRefs(menubar));
    if (!menubar) return;

    PRUint32    numMenus=0;
    menubar->GetMenuCount(numMenus);
    for (PRInt32 i = numMenus-1; i >= 0; i--)
    {
        nsCOMPtr<nsIMenu> menu;
        menubar->GetMenuAt(i, *getter_AddRefs(menu));
        if (menu)
        {
            menu->SetEnabled(enableFlag);
        }
    }
}



















NS_IMETHODIMP nsMacWindow::ConstrainPosition(PRBool aAllowSlop,
                                             PRInt32 *aX, PRInt32 *aY)
{
  if (eWindowType_popup == mWindowType || !mWindowMadeHere)
    return NS_OK;

  
  

  
  Rect portBounds;
  ::GetWindowPortBounds(mWindowPtr, &portBounds);
  short windowWidth = portBounds.right - portBounds.left;
  short windowHeight = portBounds.bottom - portBounds.top;

  
  
  Rect screenRect;
  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    PRInt32 left, top, width, height, fullHeight;

    
    
    width = windowWidth > 0 ? windowWidth : 1;
    height = windowHeight > 0 ? windowHeight : 1;
    screenmgr->ScreenForRect(*aX, *aY, width, height,
                            getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailRect(&left, &top, &width, &height);
      screen->GetRect(&left, &top, &width, &fullHeight);
      screenRect.left = left;
      screenRect.right = left+width;
      screenRect.top = top;
      screenRect.bottom = top+height;
    }
  } else
    ::GetRegionBounds(::GetGrayRgn(), &screenRect);

  if (aAllowSlop) {
    short pos;
    pos = screenRect.left;
    if (windowWidth > kWindowPositionSlop)
      pos -= windowWidth - kWindowPositionSlop;
    if (*aX < pos)
      *aX = pos;
    else if (*aX >= screenRect.right - kWindowPositionSlop)
      *aX = screenRect.right - kWindowPositionSlop;

    pos = screenRect.top;
    if (windowHeight > kWindowPositionSlop)
      pos -= windowHeight - kWindowPositionSlop;
    if (*aY < pos)
      *aY = pos;
    else if (*aY >= screenRect.bottom - kWindowPositionSlop)
      *aY = screenRect.bottom - kWindowPositionSlop;
  } else {
    if (*aX < screenRect.left)
      *aX = screenRect.left;
    else if (*aX >= screenRect.right - windowWidth)
      *aX = screenRect.right - windowWidth;

    if (*aY < screenRect.top)
      *aY = screenRect.top;
    else if (*aY >= screenRect.bottom - windowHeight)
      *aY = screenRect.bottom - windowHeight;
  }

  return NS_OK;
}









NS_IMETHODIMP nsMacWindow::Move(PRInt32 aX, PRInt32 aY)
{
  StPortSetter setOurPortForLocalToGlobal ( mWindowPtr );
  
  if (eWindowType_popup == mWindowType) {
    
    
    
    

    const PRInt32 kMoveThreshold = 2;
    Rect currBounds;
    ::GetWindowBounds ( mWindowPtr, kWindowGlobalPortRgn, &currBounds );
    if ( abs(currBounds.left-aX) > kMoveThreshold || abs(currBounds.top-aY) > kMoveThreshold ) {
      ::MoveWindow(mWindowPtr, aX, aY, false);
      
      
      PRInt32 sizeMode;
      nsBaseWidget::GetSizeMode ( &sizeMode );
      if ( sizeMode == nsSizeMode_Normal ) {
        ::GetWindowBounds ( mWindowPtr, kWindowGlobalPortRgn, &currBounds );
        ::SetWindowUserState ( mWindowPtr, &currBounds );
      }  
    }  

    return NS_OK;
  } else if (mWindowMadeHere) {
    Rect portBounds;
    ::GetWindowPortBounds(mWindowPtr, &portBounds);

    aX += mBoundsOffset.h;
    aY += mBoundsOffset.v;

    nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
    if (screenmgr) {
      nsCOMPtr<nsIScreen> screen;
      PRInt32 left, top, width, height, fullTop;
      
      width = portBounds.right - portBounds.left;
      height = portBounds.bottom - portBounds.top;
      if (height <= 0) height = 1;
      if (width <= 0) width = 1;

      screenmgr->ScreenForRect(aX, aY, width, height,
                               getter_AddRefs(screen));
      if (screen) {
        screen->GetAvailRect(&left, &top, &width, &height);
        screen->GetRect(&left, &fullTop, &width, &height);
        aY += top-fullTop;
      }
    }

    
    
    ::LocalToGlobal((Point *) &portBounds.top);
    ::LocalToGlobal((Point *) &portBounds.bottom);
    if (portBounds.left != aX || portBounds.top != aY) {
      ::MoveWindow(mWindowPtr, aX, aY, false);

      
      PRInt32 sizeMode;
      GetSizeMode(&sizeMode);
      if (sizeMode == nsSizeMode_Normal) {
        Rect newBounds;
        ::GetWindowBounds(mWindowPtr, kWindowGlobalPortRgn, &newBounds);
        ::SetWindowUserState(mWindowPtr, &newBounds);
      }
    }

    
    Inherited::Move(aX, aY);

    
    mBounds.x = 0;
    mBounds.y = 0;
  }
  return NS_OK;
}






NS_METHOD nsMacWindow::PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                   nsIWidget *aWidget, PRBool aActivate)
{
  if (aWidget) {
    WindowPtr behind = (WindowPtr)aWidget->GetNativeData(NS_NATIVE_DISPLAY);
    ::SendBehind(mWindowPtr, behind);
    ::HiliteWindow(mWindowPtr, FALSE);
  } else {
    if (::FrontWindow() != mWindowPtr)
      ::SelectWindow(mWindowPtr);
  }
  return NS_OK;
}






NS_METHOD nsMacWindow::SetSizeMode(PRInt32 aMode)
{
  nsresult rv = NS_OK;

  
  if (mZooming)
    return NS_OK;

  
  
  
  

  if (!mVisible) {
    


    mZoomOnShow = aMode == nsSizeMode_Maximized;
  } else {
    PRInt32 previousMode;
    mZooming = PR_TRUE;

    nsBaseWidget::GetSizeMode(&previousMode);
    rv = nsBaseWidget::SetSizeMode(aMode);
    if (NS_SUCCEEDED(rv)) {
      if (aMode == nsSizeMode_Minimized) {
        ::CollapseWindow(mWindowPtr, true);
      } else {
        if (aMode == nsSizeMode_Maximized) {
          CalculateAndSetZoomedSize();
          ::ZoomWindow(mWindowPtr, inZoomOut, ::FrontWindow() == mWindowPtr);
        } else {
          
          if (previousMode == nsSizeMode_Maximized)
            ::ZoomWindow(mWindowPtr, inZoomIn, ::FrontWindow() == mWindowPtr);
        }

        Rect macRect;
        ::GetWindowPortBounds(mWindowPtr, &macRect);
        Resize(macRect.right - macRect.left, macRect.bottom - macRect.top, PR_FALSE);
      }
    }

    mZooming = PR_FALSE;
  }

  return rv;
}




void
nsMacWindow::UserStateForResize()
{
  nsresult rv;
  PRInt32  currentMode;

  
  if (mZooming)
    return;

  
  rv = nsBaseWidget::GetSizeMode(&currentMode);
  if (NS_SUCCEEDED(rv) && currentMode == nsSizeMode_Normal)
    return;

  
  
  StPortSetter portState(mWindowPtr);
  StOriginSetter originState(mWindowPtr);
  Rect bounds;

  ::GetWindowPortBounds(mWindowPtr, &bounds);
  ::LocalToGlobal((Point *)&bounds.top);
  ::LocalToGlobal((Point *)&bounds.bottom);
  ::SetWindowUserState(mWindowPtr, &bounds);
  ::ZoomWindow(mWindowPtr, inZoomIn, false);

  
  nsBaseWidget::SetSizeMode(nsSizeMode_Normal);
  
  
  ReportMoveEvent();
}








NS_IMETHODIMP
nsMacWindow::CalculateAndSetZoomedSize()
{
  StPortSetter setOurPort(mWindowPtr);

  
  Rect windRect;
  ::GetWindowPortBounds(mWindowPtr, &windRect);
  ::LocalToGlobal((Point *)&windRect.top);
  ::LocalToGlobal((Point *)&windRect.bottom);

  
  short wTitleHeight;
  short wLeftBorder;
  short wRightBorder;
  short wBottomBorder;
       
  RgnHandle structRgn = ::NewRgn();
  ::GetWindowRegion(mWindowPtr, kWindowStructureRgn, structRgn);
  Rect structRgnBounds;
  ::GetRegionBounds(structRgn, &structRgnBounds);
  wTitleHeight = windRect.top - structRgnBounds.top;
  wLeftBorder = windRect.left - structRgnBounds.left;
  wRightBorder =  structRgnBounds.right - windRect.right;
  wBottomBorder = structRgnBounds.bottom - windRect.bottom;

  ::DisposeRgn(structRgn);

  windRect.top -= wTitleHeight;
  windRect.bottom += wBottomBorder;
  windRect.right += wRightBorder;
  windRect.left -= wLeftBorder;

  
  
  
  nsCOMPtr<nsIScreenManager> screenMgr = do_GetService(sScreenManagerContractID);
  if ( screenMgr ) {
    nsCOMPtr<nsIScreen> screen;
    screenMgr->ScreenForRect ( windRect.left, windRect.top, windRect.right - windRect.left, windRect.bottom - windRect.top,
                                getter_AddRefs(screen) );
    if ( screen ) {
      nsRect newWindowRect;
      screen->GetAvailRect ( &newWindowRect.x, &newWindowRect.y, &newWindowRect.width, &newWindowRect.height );
      
      
      nsCOMPtr<nsIScreen> primaryScreen;
      screenMgr->GetPrimaryScreen ( getter_AddRefs(primaryScreen) );
      if (screen == primaryScreen) {
        int iconSpace = 128;
        newWindowRect.width -= iconSpace;
      }

      Rect zoomRect;
      ::SetRect(&zoomRect,
                  newWindowRect.x + wLeftBorder,
                  newWindowRect.y + wTitleHeight,
                  newWindowRect.x + newWindowRect.width - wRightBorder,
                  newWindowRect.y + newWindowRect.height - wBottomBorder); 
      ::SetWindowStandardState ( mWindowPtr, &zoomRect );
    }
  }
  
  return NS_OK;

} 







NS_IMETHODIMP
nsMacWindow::GetMenuBar(nsIMenuBar **_retval)
{
  *_retval = mMenuBar;
  NS_IF_ADDREF(*_retval);
  return(NS_OK);
}

NS_IMETHODIMP
nsMacWindow::GetIsSheet(PRBool *_retval)
{
  *_retval = mIsSheet;
  return(NS_OK);
}












void nsMacWindow::MoveToGlobalPoint(PRInt32 aX, PRInt32 aY)
{
  PRInt32 left, top, width, height, fullTop;
  Rect portBounds;

  StPortSetter doThatThingYouDo(mWindowPtr);
  ::GetWindowPortBounds(mWindowPtr, &portBounds);

  width = portBounds.right - portBounds.left;
  height = portBounds.bottom - portBounds.top;
  ::LocalToGlobal(&topLeft(portBounds));

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    
    screenmgr->ScreenForRect(portBounds.left, portBounds.top, width, height,
                             getter_AddRefs(screen));
    if (screen) {
      screen->GetAvailRect(&left, &top, &width, &height);
      screen->GetRect(&left, &fullTop, &width, &height);
      aY -= top-fullTop;
    }
  }

  Move(aX-mBoundsOffset.h, aY-mBoundsOffset.v);
}






NS_IMETHODIMP nsMacWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
  return Resize(aWidth, aHeight, aRepaint, PR_FALSE); 
}

nsresult nsMacWindow::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint, PRBool aFromUI)
{
  if (mWindowMadeHere) {
#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
    if (mInUpdate && nsToolkit::OSXVersion() < MAC_OS_X_VERSION_10_3_HEX) {
      
      
      
      
      
      mResizeTo.width = aWidth;
      mResizeTo.height = aHeight;
      mResizeTo.repaint = aRepaint;
      mResizeTo.fromUI = aFromUI;

      mNeedsResize = PR_TRUE;

      return NS_OK;
    }
#endif

    Rect windowRect;
    if (::GetWindowBounds(mWindowPtr, kWindowContentRgn, &windowRect)
        != noErr) {
      NS_ERROR("::GetWindowBounds() didn't get window bounds");
      return NS_ERROR_FAILURE;
    }

    if (!aFromUI) {
      
      
      
      
      Rect desktopRect;
      if (NS_FAILED(GetDesktopRect(&desktopRect))) {
        return NS_ERROR_FAILURE;
      }

#if 1
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      short maxWidth  = PR_MAX(desktopRect.right  - desktopRect.left -
                                 mBoundsOffset.h,
                               windowRect.right   - windowRect.left);
      short maxHeight = PR_MAX(desktopRect.bottom - desktopRect.top  -
                                 mBoundsOffset.v,
                               windowRect.bottom  - windowRect.top);
#else
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      short maxWidth = PR_MAX(desktopRect.right, windowRect.right) -
                       PR_MAX(desktopRect.left, windowRect.left);

      
      
      
      short maxHeight = PR_MAX(desktopRect.bottom, windowRect.bottom) -
                        PR_MAX(desktopRect.top, windowRect.top);
#endif

      aWidth = PR_MIN(aWidth, maxWidth);
      aHeight = PR_MIN(aHeight, maxHeight);
    } 

    short currentWidth = windowRect.right - windowRect.left;
    short currentHeight = windowRect.bottom - windowRect.top;

    if (currentWidth != aWidth || currentHeight != aHeight) {
      if (aWidth != 0 && aHeight != 0) {
        
        mResizeIsFromUs = PR_TRUE;
        
        
        
        
        
        StRegionFromPool savedUpdateRgn;
        PRBool wasInUpdate = mInUpdate;
        if (mInUpdate)
        {
          
          
          
          ::GetPortVisibleRegion(::GetWindowPort(mWindowPtr), savedUpdateRgn);
          ::EndUpdate(mWindowPtr);
        }
          
        ::SizeWindow(mWindowPtr, aWidth, aHeight, aRepaint);

        if (wasInUpdate)
        {
          
          
          
          ::InvalWindowRgn(mWindowPtr, savedUpdateRgn);
          ::BeginUpdate(mWindowPtr);
        }

        
        PRInt32 sizeMode;
        GetSizeMode(&sizeMode);
        if (sizeMode == nsSizeMode_Normal) {
          Rect portBounds;
          ::GetWindowBounds(mWindowPtr, kWindowGlobalPortRgn, &portBounds);
          ::SetWindowUserState(mWindowPtr, &portBounds);
        }

        mResizeIsFromUs = PR_FALSE;
      } else {
        
        
        
        
        if (mVisible) {
          Show(PR_FALSE);
          mVisible = PR_TRUE;
        }
      }
    }
  } 
  Inherited::Resize(aWidth, aHeight, aRepaint);

  
  
  
  
  if (aWidth != 0 && aHeight != 0 && mVisible && !mShown)
    Show(PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP nsMacWindow::GetScreenBounds(nsRect &aRect) {
 
  nsRect localBounds;
  PRInt32 yAdjust = 0;

  GetBounds(localBounds);
  
  
  localBounds.MoveBy(-localBounds.x, -localBounds.y);
  WidgetToScreen(localBounds, aRect);

  nsCOMPtr<nsIScreenManager> screenmgr = do_GetService(sScreenManagerContractID);
  if (screenmgr) {
    nsCOMPtr<nsIScreen> screen;
    
    screenmgr->ScreenForRect(aRect.x, aRect.y, aRect.width, aRect.height,
                             getter_AddRefs(screen));
    if (screen) {
      PRInt32 left, top, width, height, fullTop;
      screen->GetAvailRect(&left, &top, &width, &height);
      screen->GetRect(&left, &fullTop, &width, &height);
      yAdjust = top-fullTop;
    }
  }
 
  aRect.MoveBy(-mBoundsOffset.h, -mBoundsOffset.v-yAdjust);

  return NS_OK;
}





PRBool nsMacWindow::OnPaint(nsPaintEvent &event)
{
  return PR_TRUE; 
}






NS_IMETHODIMP nsMacWindow::SetTitle(const nsAString& aTitle)
{
  nsAString::const_iterator begin;
  const PRUnichar *strTitle = aTitle.BeginReading(begin).get();
  CFStringRef labelRef = ::CFStringCreateWithCharacters(kCFAllocatorDefault, (UniChar*)strTitle, aTitle.Length());
  if (labelRef) {
    ::SetWindowTitleWithCFString(mWindowPtr, labelRef);
    ::CFRelease(labelRef);
  }

  return NS_OK;
}


#pragma mark -






NS_IMETHODIMP 
nsMacWindow::HandleUpdateActiveInputArea(const nsAString & text, 
                                         PRInt16 script, PRInt16 language, PRInt32 fixLen, void * hiliteRng, 
                                         OSStatus *_retval)
{
  *_retval = eventNotHandledErr;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  const nsPromiseFlatString& buffer = PromiseFlatString(text);
  
  nsresult res = mMacEventHandler->UnicodeHandleUpdateInputArea(buffer.get(), buffer.Length(), fixLen, (TextRangeArray*) hiliteRng);
  
  if (NS_SUCCEEDED(res))
    *_retval = noErr;
  return res;
}


NS_IMETHODIMP 
nsMacWindow::HandleUnicodeForKeyEvent(const nsAString & text, 
                                      PRInt16 script, PRInt16 language, void * keyboardEvent, 
                                      OSStatus *_retval)
{
  *_retval = eventNotHandledErr;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  
  
  EventRecord* eventPtr = (EventRecord*)keyboardEvent;
  const nsPromiseFlatString& buffer = PromiseFlatString(text);
  nsresult res = mMacEventHandler->HandleUKeyEvent(buffer.get(), buffer.Length(), *eventPtr);
  
  if(NS_SUCCEEDED(res))
    *_retval = noErr;
  return res;
}


NS_IMETHODIMP 
nsMacWindow::HandleOffsetToPos(PRInt32 offset, PRInt16 *pointX, PRInt16 *pointY, OSStatus *_retval)
{
  *_retval = eventNotHandledErr;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  *pointX = *pointY = 0;
  Point thePoint = {0,0};
  nsresult res = mMacEventHandler->HandleOffsetToPosition(offset, &thePoint);
  
  if(NS_SUCCEEDED(res))
    *_retval = noErr;
  *pointX = thePoint.h;
  *pointY = thePoint.v;  
  return res;
}



NS_IMETHODIMP 
nsMacWindow::HandlePosToOffset(PRInt16 currentPointX, PRInt16 currentPointY, 
                               PRInt32 *offset, PRInt16 *regionClass, OSStatus *_retval)
{
  *_retval = eventNotHandledErr;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  *_retval = noErr;
  Point thePoint;
  thePoint.h = currentPointX;
  thePoint.v = currentPointY;
  *offset = mMacEventHandler->HandlePositionToOffset(thePoint, regionClass);
  return NS_OK;
}


NS_IMETHODIMP 
nsMacWindow::HandleGetSelectedText(nsAString & selectedText, OSStatus *_retval)
{
  *_retval = noErr;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  return mMacEventHandler->HandleUnicodeGetSelectedText(selectedText);
}

#pragma mark -







NS_IMETHODIMP
nsMacWindow::DispatchEvent ( void* anEvent, PRBool *_retval )
{
  *_retval = PR_FALSE;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  *_retval = mMacEventHandler->HandleOSEvent(*NS_REINTERPRET_CAST(EventRecord*,anEvent));

  return NS_OK;
}







NS_IMETHODIMP
nsMacWindow::DispatchMenuEvent ( void* anEvent, PRInt32 aNativeResult, PRBool *_retval )
{
#if USE_MENUSELECT
  *_retval = PR_FALSE;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  *_retval = mMacEventHandler->HandleMenuCommand(*NS_REINTERPRET_CAST(EventRecord*,anEvent), aNativeResult);
#endif

  return NS_OK;
}











NS_IMETHODIMP
nsMacWindow::DragEvent(PRUint32 aMessage, PRInt16 aMouseGlobalX, PRInt16 aMouseGlobalY,
                         PRUint16 aKeyModifiers, PRBool *_retval)
{
  *_retval = PR_FALSE;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  Point globalPoint = {aMouseGlobalY, aMouseGlobalX};         
  *_retval = mMacEventHandler->DragEvent(aMessage, globalPoint, aKeyModifiers);
  
  return NS_OK;
}









NS_IMETHODIMP
nsMacWindow::Scroll ( PRBool aVertical, PRInt16 aNumLines, PRInt16 aMouseLocalX, 
                        PRInt16 aMouseLocalY, PRBool *_retval )
{
  *_retval = PR_FALSE;
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  Point localPoint = {aMouseLocalY, aMouseLocalX};
  *_retval = mMacEventHandler->Scroll(aVertical ? aNumLines : 0,
                                      aVertical ? 0 : aNumLines,
                                      PR_FALSE, localPoint, this, 0);
  return NS_OK;
}


NS_IMETHODIMP
nsMacWindow::Idle()
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}


#pragma mark -







NS_IMETHODIMP
nsMacWindow::ComeToFront()
{
  nsZLevelEvent event(PR_TRUE, NS_SETZLEVEL, this);

  event.refPoint.x = mBounds.x;
  event.refPoint.y = mBounds.y;
  event.time = PR_IntervalNow();

  event.mImmediate = PR_TRUE;

  DispatchWindowEvent(event);
  
  return NS_OK;
}


NS_IMETHODIMP nsMacWindow::ResetInputState()
{
  NS_ENSURE_TRUE(mMacEventHandler.get(), NS_ERROR_FAILURE);
  return mMacEventHandler->ResetInputState();
}

void nsMacWindow::SetIsActive(PRBool aActive)
{
  mIsActive = aActive;
}

void nsMacWindow::IsActive(PRBool* aActive)
{
  *aActive = mIsActive;
}

















typedef enum {
  kDockOrientationNone       = 0,
  kDockOrientationHorizontal = 1,
  kDockOrientationVertical   = 2
} nsMacDockOrientation;

nsresult nsMacWindow::GetDesktopRect(Rect* desktopRect)
{
  
  
  
  
  
  
  

  ::SetRect(desktopRect, 0, 0, 0, 0);
  Rect menuBarRect = {0, 0, 0, 0};
  Rect dockRect = {0, 0, 0, 0};

  Rect windowRect;
  if (::GetWindowBounds(mWindowPtr, kWindowStructureRgn, &windowRect)
      != noErr) {
    NS_ERROR("::GetWindowBounds() didn't get window bounds");
    return NS_ERROR_FAILURE;
  }

  Rect tempRect; 
                 

  PRUint32 windowOnScreens = 0; 
  Rect windowOnScreenRect = {0, 0, 0, 0}; 
  GDHandle screen = ::DMGetFirstScreenDevice(TRUE);
  nsMacDockOrientation dockOrientation = kDockOrientationNone;
  while (screen != NULL) {
    Rect screenRect = (*screen)->gdRect;
    if (::EmptyRect(&screenRect)) {
      NS_WARNING("Couldn't determine screen dimensions");
      continue;
    }

    ::UnionRect(desktopRect, &screenRect, desktopRect);

    if (::SectRect(&screenRect, &windowRect, &tempRect)) {
      
      
      
      
      
      windowOnScreens++;
      windowOnScreenRect = screenRect;
    }

    
    
    
    Rect availableRect;
    if (::GetAvailableWindowPositioningBounds(screen, &availableRect)
        != noErr) {
      NS_ERROR("::GetAvailableWindowPositioningBounds couldn't determine" \
               "available screen dimensions");
      return NS_ERROR_FAILURE;
    }

    
    
    
    if (availableRect.top > screenRect.top) {
      ::SetRect(&menuBarRect, screenRect.left, screenRect.top,
                              screenRect.right, availableRect.top);
    }

    
    
    
    
    
    
    
    if (availableRect.bottom < screenRect.bottom) {
      
      ::SetRect(&dockRect, availableRect.left, availableRect.bottom,
                           availableRect.right, screenRect.bottom);
      dockOrientation = kDockOrientationHorizontal;
    }
    else if (availableRect.right < screenRect.right) {
      
      ::SetRect(&dockRect, availableRect.right, availableRect.top,
                           screenRect.right, availableRect.bottom);
      dockOrientation = kDockOrientationVertical;
    }
    else if (availableRect.left > screenRect.left) {
      
      ::SetRect(&dockRect, screenRect.left, availableRect.top,
                           availableRect.left, availableRect.bottom);
      dockOrientation = kDockOrientationVertical;
    }

    screen = ::DMGetNextScreenDevice(screen, TRUE);
  }

  if (windowOnScreens == 1) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    *desktopRect = windowOnScreenRect;
  }

  if (::EmptyRect(desktopRect)) {
    NS_ERROR("Couldn't determine desktop dimensions\n");
    return NS_ERROR_FAILURE;
  }

  
  
  
  

  

  if (::SectRect(desktopRect, &menuBarRect, &tempRect) &&
      menuBarRect.top == desktopRect->top) {
    
    
    
    
    
    
    desktopRect->top += menuBarRect.bottom;
  }

  if (dockOrientation != kDockOrientationNone &&
      ::SectRect(desktopRect, &dockRect, &tempRect)) {
    

    
    
    
    
    switch (dockOrientation) {
      case kDockOrientationVertical:
        
        
        if (dockRect.left == desktopRect->left) {
          desktopRect->left += dockRect.right - dockRect.left;
        }
        else if (dockRect.right == desktopRect->right) {
          desktopRect->right -= dockRect.right - dockRect.left;
        }
        break;

      case kDockOrientationHorizontal:
        
        
        if (dockRect.bottom == desktopRect->bottom) {
          desktopRect->bottom -= dockRect.bottom - dockRect.top;
        }
        break;

      default:
        break;
    } 
  } 

  
  
  return NS_OK;
}











NS_IMETHODIMP nsMacWindow::GetChildSheet(PRBool aShown, nsMacWindow** _retval)
{
  nsIWidget* child = GetFirstChild();

  while (child) {
    
    nsWindow* window = NS_STATIC_CAST(nsWindow*, child);

    nsWindowType type;
    if (NS_SUCCEEDED(window->GetWindowType(type)) &&
        type == eWindowType_sheet) {
      
      nsMacWindow* macWindow = NS_STATIC_CAST(nsMacWindow*, window);

      if ((aShown && macWindow->mShown) ||
          (!aShown && macWindow->mSheetNeedsShow)) {
        *_retval = macWindow;
        return NS_OK;
      }
    }

    child = child->GetNextSibling();
  }

  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsMacWindow::Update()
{
  nsresult rv = Inherited::Update();

#if MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3
  if (NS_SUCCEEDED(rv) && mNeedsResize) {
    
    
    
    
    mNeedsResize = PR_FALSE;

    Resize(mResizeTo.width, mResizeTo.height,
           mResizeTo.repaint, mResizeTo.fromUI);
  }
#endif

  return rv;
}

pascal OSStatus
nsMacWindow::KeyEventHandler(EventHandlerCallRef aHandlerCallRef,
                             EventRef            aEvent,
                             void*               aUserData)
{
  nsMacWindow* self = NS_STATIC_CAST(nsMacWindow*, aUserData);
  NS_ASSERTION(self, "No self?");
  NS_ASSERTION(self->mMacEventHandler.get(), "No mMacEventHandler?");

  PRBool handled = PR_FALSE;

  EventKind kind = ::GetEventKind(aEvent);
  if (kind == kEventRawKeyModifiersChanged)
    handled = self->mMacEventHandler->HandleKeyModifierEvent(aHandlerCallRef,
                                                             aEvent);
  else
    handled = self->mMacEventHandler->HandleKeyUpDownEvent(aHandlerCallRef,
                                                           aEvent);

  if (!handled)
    return eventNotHandledErr;

  return noErr;
}

NS_IMETHODIMP
nsMacWindow::GetEventDispatchHandler(nsMacEventDispatchHandler** aEventDispatchHandler) {
  *aEventDispatchHandler = mMacEventHandler->GetEventDispatchHandler();
  return NS_OK;
}
