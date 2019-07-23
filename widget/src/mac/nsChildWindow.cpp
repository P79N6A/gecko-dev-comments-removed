




































#include "nsChildWindow.h"
#include "nsCOMPtr.h"
#include "nsRegionPool.h"





nsChildWindow::nsChildWindow() : nsWindow()
{
	WIDGET_SET_CLASSNAME("nsChildWindow");
	mClipChildren = PR_FALSE;
	mClipSiblings = PR_FALSE;
}


nsChildWindow::~nsChildWindow()
{
}






nsresult nsChildWindow::StandardCreate(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData,
                      nsNativeWidget aNativeParent)	
{
	if (aInitData)
	{
		mClipChildren = aInitData->clipChildren;
		mClipSiblings = aInitData->clipSiblings;
	}

	return Inherited::StandardCreate(aParent,
                      aRect,
                      aHandleEventFunction,
                      aContext,
                      aAppShell,
                      aToolkit,
                      aInitData,
                      aNativeParent);
}






void nsChildWindow::CalcWindowRegions()
{
  Inherited::CalcWindowRegions();

  
  if (mClipSiblings && mParent && !mIsTopWidgetWindow) {
    
    nsWindow* sibling = NS_STATIC_CAST(nsWindow*, mParent->GetLastChild());
    if (!sibling)
      return;

    StRegionFromPool siblingRgn;
    if (siblingRgn == nsnull)
      return;

    do {
      if (sibling == NS_STATIC_CAST(nsWindow*, this))
        break;

      PRBool visible;
      sibling->IsVisible(visible);
      if (visible && !sibling->IsTopLevelWidgetWindow()) {
        
        
        nsRect siblingRect;
        sibling->GetBounds(siblingRect);

        
        siblingRect.MoveBy(-mBounds.x, -mBounds.y);

        Rect macRect;
        ::SetRect(&macRect, siblingRect.x, siblingRect.y,
                  siblingRect.XMost(), siblingRect.YMost());
        ::RectRgn(siblingRgn, &macRect);
        ::DiffRgn(mWindowRegion, siblingRgn, mWindowRegion);
        ::DiffRgn(mVisRegion, siblingRgn, mVisRegion);
      }
      sibling = NS_STATIC_CAST(nsWindow*, sibling->GetPrevSibling());
    } while (sibling);
  }
}
