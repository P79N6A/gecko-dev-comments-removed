






































#include "nsScrollbar.h"
#include "nsGUIEvent.h"

#define mScroll  ((QScrollBar*)mWidget)

nsQScrollBar::nsQScrollBar(int aMinValue, int aMaxValue, int aLineStep,
                           int aPageStep, int aValue, Orientation aOrientation,
                           QWidget *aParent, const char *aName)
    : QScrollBar(aMinValue, aMaxValue, aLineStep, aPageStep, aValue,
                 aOrientation, aParent, aName)
{
    qDebug("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
    connect(this,SIGNAL(valueChanged(int)),
            SLOT(slotValueChanged(int)));
}

void nsQScrollBar::slotValueChanged(int aValue)
{
    ScrollBarMoved(NS_SCROLLBAR_POS,aValue);
}

void nsQScrollBar::ScrollBarMoved(int aMessage,int aValue)
{
    nsScrollbarEvent nsEvent(PR_TRUE, aMessage, nsnull);

    nsEvent.eventStructType = NS_SCROLLBAR_EVENT;
    nsEvent.position        = aValue;

    
    
}




NS_IMPL_ADDREF_INHERITED(nsNativeScrollbar,nsCommonWidget)
NS_IMPL_RELEASE_INHERITED(nsNativeScrollbar,nsCommonWidget)
NS_IMPL_QUERY_INTERFACE2(nsNativeScrollbar,nsINativeScrollbar,nsIWidget)

nsNativeScrollbar::nsNativeScrollbar()
    : nsCommonWidget(),
      nsINativeScrollbar()
{

    qDebug("YESSSSSSSSSSSSSSSSSSSSSSSSSSSSSSS");
    mOrientation = true ? QScrollBar::Vertical : QScrollBar::Horizontal;
    mLineStep = 1;
    mPageStep = 10;
    mMaxValue = 100;
    mValue    = 0;
    mListenForResizes = PR_TRUE;
}

nsNativeScrollbar::~nsNativeScrollbar()
{
}

NS_METHOD
nsNativeScrollbar::SetMaxRange(PRUint32 aEndRange)
{
    mMaxValue = aEndRange;

    mScroll->setRange(0,mMaxValue - mPageStep);

    return NS_OK;
}

NS_METHOD
nsNativeScrollbar::GetMaxRange(PRUint32 *aMaxRange)
{
    *aMaxRange = mMaxValue;
    return NS_OK;
}

NS_METHOD nsNativeScrollbar::SetPosition(PRUint32 aPos)
{
    mValue = aPos;

    mScroll->setValue(mValue);

    return NS_OK;
}

NS_METHOD
nsNativeScrollbar::GetPosition(PRUint32 *aPos)
{
    *aPos = mValue;
    return NS_OK;
}




NS_METHOD
nsNativeScrollbar::SetLineIncrement(PRUint32 aLineIncrement)
{
    if (aLineIncrement > 0) {
        mLineStep = aLineIncrement;

        mScroll->setSteps(mLineStep,mPageStep);
    }
    return NS_OK;
}




NS_METHOD
nsNativeScrollbar::GetLineIncrement(PRUint32 *aLineInc)
{
    *aLineInc = mLineStep;
    return NS_OK;
}

#if 0
NS_METHOD
nsNativeScrollbar::SetParameters(PRUint32 aMaxRange,PRUint32 aThumbSize,
                           PRUint32 aPosition,PRUint32 aLineIncrement)
{
    mPageStep = (aThumbSize > 0) ? aThumbSize : 1;
    mValue    = (aPosition > 0) ? aPosition : 0;
    mLineStep = (aLineIncrement > 0) ? aLineIncrement : 1;
    mMaxValue = (aMaxRange > 0) ? aMaxRange : 10;

    mScroll->setValue(mValue);
    mScroll->setSteps(mLineStep,mPageStep);
    mScroll->setRange(0,mMaxValue - mPageStep);
    return NS_OK;
}
#endif




PRBool nsNativeScrollbar::OnScroll(nsScrollbarEvent &aEvent,PRUint32 cPos)
{
    nsEventStatus result = nsEventStatus_eIgnore;

    switch (aEvent.message) {
        
    case NS_SCROLLBAR_LINE_NEXT:
        mScroll->addLine();
        mValue = mScroll->value();

        
        
        if (mEventCallback) {
            aEvent.position = (PRUint32)mValue;
            result = (*mEventCallback)(&aEvent);
            mValue = aEvent.position;
        }
        break;

        
    case NS_SCROLLBAR_LINE_PREV:
        mScroll->subtractLine();
        mValue = mScroll->value();

        
        
        if (mEventCallback) {
            aEvent.position = (PRUint32)mValue;
            result = (*mEventCallback)(&aEvent);
            mValue = aEvent.position;
        }
        break;

        
    case NS_SCROLLBAR_PAGE_NEXT:
        mScroll->addPage();
        mValue = mScroll->value();

        
        
        if (mEventCallback) {
            aEvent.position = (PRUint32)mValue;
            result = (*mEventCallback)(&aEvent);
            mValue = aEvent.position;
        }
        break;

        
    case NS_SCROLLBAR_PAGE_PREV:
        mScroll->subtractPage();
        mValue = mScroll->value();

        
        
        if (mEventCallback) {
            aEvent.position = (PRUint32)mValue;
            result = (*mEventCallback)(&aEvent);
            mValue = aEvent.position;
        }
        break;

        
        
    case NS_SCROLLBAR_POS:
        mValue = cPos;

        
        
        if (mEventCallback) {
            aEvent.position = (PRUint32)mValue;
            result = (*mEventCallback)(&aEvent);
            mValue = aEvent.position;
        }
        break;
    }

    return ignoreEvent(result);
}

QWidget*
nsNativeScrollbar::createQWidget(QWidget *parent, nsWidgetInitData *)
{
    mWidget =  new nsQScrollBar(0, mMaxValue, mLineStep, mPageStep,
                                mValue, mOrientation, parent);
    mWidget->setMouseTracking(PR_TRUE);

    return mWidget;
}


NS_IMETHODIMP
nsNativeScrollbar::SetContent(nsIContent *content, nsISupports *scrollbar, nsIScrollbarMediator *mediator)
{
    qDebug("XXXXXXXX SetContent");
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsNativeScrollbar::GetNarrowSize(PRInt32 *aNarrowSize)
{
    qDebug("XXXXXXXXXXXX GetNarrowSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsNativeScrollbar::GetViewSize(PRUint32 *aViewSize)
{
    qDebug("XXXXXXXXX GetViewSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
nsNativeScrollbar::SetViewSize(PRUint32 aViewSize)
{
    qDebug("XXXXXXXXXXX SetViewSize");
    return NS_ERROR_NOT_IMPLEMENTED;
}
