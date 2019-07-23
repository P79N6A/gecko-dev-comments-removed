





































#include "nsCommonWidget.h"
#include "nsGtkKeyUtils.h"

nsCommonWidget::nsCommonWidget()
{
    mIsTopLevel       = PR_FALSE;
    mIsDestroyed      = PR_FALSE;
    mNeedsResize      = PR_FALSE;
    mNeedsMove        = PR_FALSE;
    mListenForResizes = PR_FALSE;
    mIsShown          = PR_FALSE;
    mNeedsShow        = PR_FALSE;
    mEnabled          = PR_TRUE;
    mCreated          = PR_FALSE;
    mPlaced           = PR_FALSE;

    mPreferredWidth   = 0;
    mPreferredHeight  = 0;
}

nsCommonWidget::~nsCommonWidget()
{
}

nsIWidget *
nsCommonWidget::GetParent(void)
{
    return mParent;
}

void
nsCommonWidget::CommonCreate(nsIWidget *aParent, PRBool aListenForResizes)
{
    mParent = aParent;
    mListenForResizes = aListenForResizes;
    mCreated = PR_TRUE;
}

void
nsCommonWidget::InitButtonEvent(nsMouseEvent &aEvent,
                                GdkEventButton *aGdkEvent)
{
    aEvent.refPoint.x = nscoord(aGdkEvent->x);
    aEvent.refPoint.y = nscoord(aGdkEvent->y);

    aEvent.isShift   = (aGdkEvent->state & GDK_SHIFT_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isControl = (aGdkEvent->state & GDK_CONTROL_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isAlt     = (aGdkEvent->state & GDK_MOD1_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isMeta    = (aGdkEvent->state & GDK_MOD4_MASK)
        ? PR_TRUE : PR_FALSE;

    aEvent.time = aGdkEvent->time;

    switch (aGdkEvent->type) {
    case GDK_2BUTTON_PRESS:
        aEvent.clickCount = 2;
        break;
    case GDK_3BUTTON_PRESS:
        aEvent.clickCount = 3;
        break;
        
    default:
        aEvent.clickCount = 1;
    }
}

void
nsCommonWidget::InitMouseScrollEvent(nsMouseScrollEvent &aEvent,
                                     GdkEventScroll *aGdkEvent)
{
    switch (aGdkEvent->direction) {
    case GDK_SCROLL_UP:
        aEvent.scrollFlags = nsMouseScrollEvent::kIsVertical;
        aEvent.delta = -3;
        break;
    case GDK_SCROLL_DOWN:
        aEvent.scrollFlags = nsMouseScrollEvent::kIsVertical;
        aEvent.delta = 3;
        break;
    case GDK_SCROLL_LEFT:
        aEvent.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
        aEvent.delta = -3;
        break;
    case GDK_SCROLL_RIGHT:
        aEvent.scrollFlags = nsMouseScrollEvent::kIsHorizontal;
        aEvent.delta = 3;
        break;
    }

    aEvent.refPoint.x = nscoord(aGdkEvent->x);
    aEvent.refPoint.y = nscoord(aGdkEvent->y);

    aEvent.isShift   = (aGdkEvent->state & GDK_SHIFT_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isControl = (aGdkEvent->state & GDK_CONTROL_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isAlt     = (aGdkEvent->state & GDK_MOD1_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isMeta    = (aGdkEvent->state & GDK_MOD4_MASK)
        ? PR_TRUE : PR_FALSE;
    
    aEvent.time = aGdkEvent->time;
}

void
nsCommonWidget::InitKeyEvent(nsKeyEvent &aEvent, GdkEventKey *aGdkEvent)
{
    aEvent.keyCode   = GdkKeyCodeToDOMKeyCode(aGdkEvent->keyval);
    aEvent.isShift   = (aGdkEvent->state & GDK_SHIFT_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isControl = (aGdkEvent->state & GDK_CONTROL_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isAlt     = (aGdkEvent->state & GDK_MOD1_MASK)
        ? PR_TRUE : PR_FALSE;
    aEvent.isMeta    = (aGdkEvent->state & GDK_MOD4_MASK)
        ? PR_TRUE : PR_FALSE;
    
    
    
    
    aEvent.nativeMsg = (void *)aGdkEvent;

    aEvent.time      = aGdkEvent->time;
}

void
nsCommonWidget::DispatchGotFocusEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_GOTFOCUS, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsCommonWidget::DispatchLostFocusEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_LOSTFOCUS, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsCommonWidget::DispatchActivateEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_ACTIVATE, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsCommonWidget::DispatchDeactivateEvent(void)
{
    nsGUIEvent event(PR_TRUE, NS_DEACTIVATE, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

void
nsCommonWidget::DispatchResizeEvent(nsRect &aRect, nsEventStatus &aStatus)
{
    nsSizeEvent event(PR_TRUE, NS_SIZE, this);

    event.windowSize = &aRect;
    event.refPoint.x = aRect.x;
    event.refPoint.y = aRect.y;
    event.mWinWidth = aRect.width;
    event.mWinHeight = aRect.height;

    nsEventStatus status;
    DispatchEvent(&event, status); 
}

NS_IMETHODIMP
nsCommonWidget::DispatchEvent(nsGUIEvent *aEvent,
                              nsEventStatus &aStatus)
{
    aStatus = nsEventStatus_eIgnore;

    
    NS_ADDREF(aEvent->widget);

    
    if (mEventCallback)
        aStatus = (* mEventCallback)(aEvent);

    
    if ((aStatus != nsEventStatus_eIgnore) && mEventListener)
        aStatus = mEventListener->ProcessEvent(*aEvent);

    NS_IF_RELEASE(aEvent->widget);

    return NS_OK;
}

NS_IMETHODIMP
nsCommonWidget::Show(PRBool aState)
{
    mIsShown = aState;

    LOG(("nsCommonWidget::Show [%p] state %d\n", (void *)this, aState));

    
    
    
    if ((aState && !AreBoundsSane()) || !mCreated) {
        LOG(("\tbounds are insane or window hasn't been created yet\n"));
        mNeedsShow = PR_TRUE;
        return NS_OK;
    }

    
    if (!aState)
        mNeedsShow = PR_FALSE;

    
    
    if (aState) {
        if (mNeedsMove) {
            LOG(("\tresizing\n"));
            NativeResize(mBounds.x, mBounds.y, mBounds.width, mBounds.height,
                         PR_FALSE);
        } else if (mNeedsResize) {
            NativeResize(mBounds.width, mBounds.height, PR_FALSE);
        }
    }

    NativeShow(aState);

    return NS_OK;
}

NS_IMETHODIMP
nsCommonWidget::Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint)
{
    mBounds.width = aWidth;
    mBounds.height = aHeight;

    if (!mCreated)
        return NS_OK;

    
    
    

    
    if (mIsShown) {
        
        if (AreBoundsSane()) {
            
            

            
            
            
            
            if (mIsTopLevel || mNeedsShow)
                NativeResize(mBounds.x, mBounds.y,
                             mBounds.width, mBounds.height, aRepaint);
            else
                NativeResize(mBounds.width, mBounds.height, aRepaint);

            
            if (mNeedsShow)
                NativeShow(PR_TRUE);
        }
        else {
            
            
            
            
            
            
            if (!mNeedsShow) {
                mNeedsShow = PR_TRUE;
                NativeShow(PR_FALSE);
            }
        }
    }
    
    
    else {
        if (AreBoundsSane() && mListenForResizes) {
            
            
            
            NativeResize(aWidth, aHeight, aRepaint);
        }
        else {
            mNeedsResize = PR_TRUE;
        }
    }

    
    if (mIsTopLevel || mListenForResizes) {
        nsRect rect(mBounds.x, mBounds.y, aWidth, aHeight);
        nsEventStatus status;
        DispatchResizeEvent(rect, status);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCommonWidget::Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight,
                       PRBool aRepaint)
{
    mBounds.x = aX;
    mBounds.y = aY;
    mBounds.width = aWidth;
    mBounds.height = aHeight;

    mPlaced = PR_TRUE;

    if (!mCreated)
        return NS_OK;

    
    
    

    
    if (mIsShown) {
        
        if (AreBoundsSane()) {
            
            NativeResize(aX, aY, aWidth, aHeight, aRepaint);
            
            if (mNeedsShow)
                NativeShow(PR_TRUE);
        }
        else {
            
            
            
            
            
            
            if (!mNeedsShow) {
                mNeedsShow = PR_TRUE;
                NativeShow(PR_FALSE);
            }
        }
    }
    
    
    else {
        if (AreBoundsSane() && mListenForResizes){
            
            
            
            NativeResize(aX, aY, aWidth, aHeight, aRepaint);
        }
        else {
            mNeedsResize = PR_TRUE;
            mNeedsMove = PR_TRUE;
        }
    }

    if (mIsTopLevel || mListenForResizes) {
        
        nsRect rect(aX, aY, aWidth, aHeight);
        nsEventStatus status;
        DispatchResizeEvent(rect, status);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsCommonWidget::GetPreferredSize(PRInt32 &aWidth,
                                 PRInt32 &aHeight)
{
    aWidth  = mPreferredWidth;
    aHeight = mPreferredHeight;
    return (mPreferredWidth != 0 && mPreferredHeight != 0) ? 
        NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsCommonWidget::SetPreferredSize(PRInt32 aWidth,
                                 PRInt32 aHeight)
{
    mPreferredWidth  = aWidth;
    mPreferredHeight = aHeight;
    return NS_OK;
}

NS_IMETHODIMP
nsCommonWidget::Enable(PRBool aState)
{
    mEnabled = aState;

    return NS_OK;
}

NS_IMETHODIMP
nsCommonWidget::IsEnabled(PRBool *aState)
{
    *aState = mEnabled;

    return NS_OK;
}

void
nsCommonWidget::OnDestroy(void)
{
    if (mOnDestroyCalled)
        return;

    mOnDestroyCalled = PR_TRUE;

    
    nsBaseWidget::OnDestroy();

    
    mParent = nsnull;

    nsCOMPtr<nsIWidget> kungFuDeathGrip = this;

    nsGUIEvent event(PR_TRUE, NS_DESTROY, this);
    nsEventStatus status;
    DispatchEvent(&event, status);
}

PRBool
nsCommonWidget::AreBoundsSane(void)
{
    if (mBounds.width > 0 && mBounds.height > 0)
        return PR_TRUE;

    return PR_FALSE;
}
