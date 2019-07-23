





































#include <ControlDefinitions.h>

#include "nsNativeScrollbar.h"
#include "nsIDeviceContext.h"

#include "nsWidgetAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIDOMElement.h"
#include "nsIScrollbarMediator.h"


inline void BoundsCheck(PRInt32 low, PRUint32& value, PRUint32 high)
{
  if ((PRInt32) value < low)
    value = low;
  if (value > high)
    value = high;
}







class StNativeControlActionProcOwner {
public:
  
  StNativeControlActionProcOwner ( )
  {
    sControlActionProc = NewControlActionUPP(nsNativeScrollbar::ScrollActionProc);
    NS_ASSERTION(sControlActionProc, "Couldn't create live scrolling action proc");
  }
  ~StNativeControlActionProcOwner ( )
  {
    if ( sControlActionProc )
      DisposeControlActionUPP(sControlActionProc);
  }

  ControlActionUPP ActionProc() { return sControlActionProc; }
  
private:
  ControlActionUPP sControlActionProc;  
};


static ControlActionUPP 
ScrollbarActionProc( )
{
  static StNativeControlActionProcOwner sActionProcOwner;
  return sActionProcOwner.ActionProc();
}


NS_IMPL_ISUPPORTS_INHERITED1(nsNativeScrollbar, nsWindow, nsINativeScrollbar)

nsNativeScrollbar::nsNativeScrollbar()
  : nsMacControl()
  , mContent(nsnull)
  , mMediator(nsnull)
  , mScrollbar(nsnull)
  , mMaxValue(0)
  , mVisibleImageSize(0)
  , mLineIncrement(0)
  , mMouseDownInScroll(PR_FALSE)
  , mClickedPartCode(0)
{
  mMax = 0;   

  WIDGET_SET_CLASSNAME("nsNativeScrollbar");
  SetControlType(kControlScrollBarLiveProc);
}


nsNativeScrollbar::~nsNativeScrollbar()
{
}








NS_IMETHODIMP
nsNativeScrollbar::Destroy()
{
  if (mMouseDownInScroll)
  {
    ::PostEvent(mouseUp, 0);
  }
  return nsMacControl::Destroy();
}







pascal void
nsNativeScrollbar::ScrollActionProc(ControlHandle ctrl, ControlPartCode part)
{
  nsNativeScrollbar* self = (nsNativeScrollbar*)(::GetControlReference(ctrl));
  NS_ASSERTION(self, "NULL nsNativeScrollbar");
  if ( self )
    self->DoScrollAction(part);
}









void
nsNativeScrollbar::DoScrollAction(ControlPartCode part)
{
  PRUint32 oldPos, newPos;
  PRUint32 incr;
  PRUint32 visibleImageSize;

  if (mOnDestroyCalled)
    return;

  nsCOMPtr<nsIWidget> parent ( GetParent() );
  if (!parent)
  {
    
    
    NS_ASSERTION(parent, "no parent in DoScrollAction");
    return;
  }

  if (!IsQDStateOK()) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    EndDraw();
    StartDraw();
    ::PostEvent(mouseDown, 0);
    return;
  }

  GetPosition(&oldPos);
  GetLineIncrement(&incr);
  GetViewSize(&visibleImageSize);

  PRBool buttonPress = PR_FALSE;

  switch (part)
  {
    case kControlUpButtonPart:
      newPos = oldPos - (mLineIncrement ? mLineIncrement : 1);
      buttonPress = PR_TRUE;
      break;
    case kControlDownButtonPart:
      newPos = oldPos + (mLineIncrement ? mLineIncrement : 1);
      buttonPress = PR_TRUE;
      break;
    
    case kControlPageUpPart:
      newPos = oldPos - visibleImageSize;
      break;
    case kControlPageDownPart:
      newPos = oldPos + visibleImageSize;
      break;

    case kControlIndicatorPart:
      newPos = ::GetControl32BitValue(GetControl());
      break;

    default:
      
      return;
  }

  if (buttonPress) {
    
    
    
    
    
    
    
    
    if (mMediator) {
      BoundsCheck(0, newPos, mMaxValue);
      mMediator->ScrollbarButtonPressed(mScrollbar, oldPos, newPos);
    }
    else {
      UpdateContentPosition(newPos);
    }
  }
  else {
    
    
    
    
    
    
    
    
    UpdateContentPosition(newPos);
    if (mMediator) {
      PRInt32 np = newPos;
      if (np < 0) {
        np = 0;
      }
      mMediator->PositionChanged(mScrollbar, oldPos, np);
    }
  }

  EndDraw();
    
  
  
  
  
  parent->Update();
  parent->Validate();

  StartDraw();
}








void
nsNativeScrollbar::UpdateContentPosition(PRUint32 inNewPos)
{
  if ( (PRInt32)inNewPos == mValue || !mContent )   
    return;

  
  BoundsCheck(0, inNewPos, mMaxValue);

  
  nsAutoString buffer;
  buffer.AppendInt(inNewPos);
  
  mContent->SetAttr(kNameSpaceID_None, nsWidgetAtoms::curpos, buffer, PR_TRUE);
  SetPosition(inNewPos);
}







ControlPartCode
nsNativeScrollbar::GetControlHiliteState()
{
  if (mMaxValue == 0)
    return kControlInactivePart;
  
  return Inherited::GetControlHiliteState();
}






 
PRBool
nsNativeScrollbar::DispatchMouseEvent(nsMouseEvent &aEvent)
{
  PRBool eatEvent = PR_FALSE;
  switch (aEvent.message)
  {
    case NS_MOUSE_DOUBLECLICK:
    case NS_MOUSE_BUTTON_DOWN:
      if (aEvent.button == nsMouseEvent::eLeftButton) {
        mMouseDownInScroll = PR_TRUE;
        NS_ASSERTION(this != 0, "NULL nsNativeScrollbar2");
        ::SetControlReference(mControl, (UInt32) this);
        StartDraw();
        {
          Point thePoint;
          thePoint.h = aEvent.refPoint.x;
          thePoint.v = aEvent.refPoint.y;
          mClickedPartCode = ::TestControl(mControl, thePoint);
          if (mClickedPartCode > 0)
            ::HiliteControl(mControl, mClickedPartCode);
  
          switch (mClickedPartCode)
          {
            case kControlUpButtonPart:
            case kControlDownButtonPart:
            case kControlPageUpPart:
            case kControlPageDownPart:
            case kControlIndicatorPart:
              
              
              
              
              
              ::TrackControl(mControl, thePoint, ScrollbarActionProc());
              ::HiliteControl(mControl, 0);
              
              
              
              eatEvent = PR_TRUE;
              break;
          }
          SetPosition(mValue);
        }
        EndDraw();
      }
      break;


    case NS_MOUSE_BUTTON_UP:
      if (aEvent.button == nsMouseEvent::eLeftButton) {
        mMouseDownInScroll = PR_FALSE;
        mClickedPartCode = 0;
      }
      break;

    case NS_MOUSE_EXIT:
      if (mWidgetArmed)
      {
        StartDraw();
        ::HiliteControl(mControl, 0);
        EndDraw();
      }
      break;

    case NS_MOUSE_ENTER:
      if (mWidgetArmed)
      {
        StartDraw();
        ::HiliteControl(mControl, mClickedPartCode);
        EndDraw();
      }
      break;
  }

  if (eatEvent)
    return PR_TRUE;
  return (Inherited::DispatchMouseEvent(aEvent));

}








NS_IMETHODIMP
nsNativeScrollbar::SetMaxRange(PRUint32 aEndRange)
{
  if ((PRInt32)aEndRange < 0)
    aEndRange = 0;

  mMaxValue = aEndRange;

  if ( GetControl() ) {
    StartDraw();
    ::SetControl32BitMaximum(GetControl(), mMaxValue);
    EndDraw();
  }
  return NS_OK;
}







NS_IMETHODIMP
nsNativeScrollbar::GetMaxRange(PRUint32* aMaxRange)
{
  *aMaxRange = mMaxValue;
  return NS_OK;
}







NS_IMETHODIMP
nsNativeScrollbar::SetPosition(PRUint32 aPos)
{
  if ((PRInt32)aPos < 0)
    aPos = 0;

  PRInt32 oldValue = mValue;
  
  
  
  
  
  
  
  
  mValue = aPos;
  
  
  
  if ( mValue != oldValue )
    Invalidate(PR_TRUE);
  
  return NS_OK;
}







NS_IMETHODIMP
nsNativeScrollbar::GetPosition(PRUint32* aPos)
{
  *aPos = mValue;
  return NS_OK;
}










NS_IMETHODIMP
nsNativeScrollbar::SetViewSize(PRUint32 aSize)
{
  if ((PRInt32)aSize < 0)
    aSize = 0;

  mVisibleImageSize = aSize;
    
  if ( GetControl() )  {
    StartDraw();
    ::SetControlViewSize(GetControl(), mVisibleImageSize);
    EndDraw();
  }
  return NS_OK;
}







NS_IMETHODIMP
nsNativeScrollbar::GetViewSize(PRUint32* aSize)
{
  *aSize = mVisibleImageSize;
  return NS_OK;
}







NS_IMETHODIMP
nsNativeScrollbar::SetLineIncrement(PRUint32 aLineIncrement)
{
  mLineIncrement  = (((int)aLineIncrement) > 0 ? aLineIncrement : 1);
  return NS_OK;
}







NS_IMETHODIMP
nsNativeScrollbar::GetLineIncrement(PRUint32* aLineIncrement)
{
  *aLineIncrement = mLineIncrement;
  return NS_OK;
}









NS_IMETHODIMP
nsNativeScrollbar::GetNarrowSize(PRInt32* outSize)
{
  if ( *outSize )
    return NS_ERROR_FAILURE;
  SInt32 width = 0;
  ::GetThemeMetric(kThemeMetricScrollBarWidth, &width);
  *outSize = width;
  return NS_OK;
}









NS_IMETHODIMP
nsNativeScrollbar::SetContent(nsIContent* inContent, nsISupports* inScrollbar, 
                              nsIScrollbarMediator* inMediator)
{
  mContent = inContent;
  mMediator = inMediator;
  mScrollbar = inScrollbar;
  return NS_OK;
}
