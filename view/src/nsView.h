




































#ifndef nsView_h___
#define nsView_h___

#include "nsIView.h"
#include "nsIWidget.h"
#include "nsRegion.h"
#include "nsRect.h"
#include "nsCRT.h"
#include "nsIFactory.h"
#include "nsIViewObserver.h"
#include "nsEvent.h"
#include <stdio.h>



class nsIRegion;
class nsIRenderingContext;
class nsIViewManager;
class nsViewManager;
class nsZPlaceholderView;




#define NS_VIEW_FLAG_CONTAINS_PLACEHOLDER 0x0100



#define NS_VIEW_FLAG_DONT_CHECK_CHILDREN  0x0200




#define NS_VIEW_FLAG_CLIP_CHILDREN_TO_BOUNDS      0x0800



#define NS_VIEW_FLAG_CLIP_PLACEHOLDERS_TO_BOUNDS  0x1000


#define NS_VIEW_FLAG_HAS_POSITIONED_WIDGET 0x2000

class nsView : public nsIView
{
public:
  nsView(nsViewManager* aViewManager = nsnull,
         nsViewVisibility aVisibility = nsViewVisibility_kShow);

  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  





  virtual void SetPosition(nscoord aX, nscoord aY);
  




  virtual void SetDimensions(const nsRect &aRect, PRBool aPaint = PR_TRUE,
                             PRBool aResizeWidget = PR_TRUE);
  void GetDimensions(nsRect &aRect) const { aRect = mDimBounds; aRect.x -= mPosX; aRect.y -= mPosY; }
  void GetDimensions(nsSize &aSize) const { aSize.width = mDimBounds.width; aSize.height = mDimBounds.height; }

  



  virtual PRBool IsZPlaceholderView() const { return PR_FALSE; }

  




  NS_IMETHOD  SetVisibility(nsViewVisibility visibility);

  








  void SetZIndex(PRBool aAuto, PRInt32 aZIndex, PRBool aTopMost);

  







  NS_IMETHOD  SetFloating(PRBool aFloatingView);
  








  NS_IMETHOD  SetWidget(nsIWidget *aWidget);

  
  static nsView* GetViewFor(nsIWidget* aWidget) {
    return NS_STATIC_CAST(nsView*, nsIView::GetViewFor(aWidget));
  }

  
  
  void DropMouseGrabbing();

public:
  
  nsZPlaceholderView* GetZParent() const { return mZParent; }
  
  
  nsView* GetFirstChild() const { return mFirstChild; }
  nsView* GetNextSibling() const { return mNextSibling; }
  nsView* GetParent() const { return mParent; }
  nsViewManager* GetViewManager() const { return mViewManager; }
  
  PRInt32 GetZIndex() const { return mZIndex; }
  PRBool GetZIndexIsAuto() const { return (mVFlags & NS_VIEW_FLAG_AUTO_ZINDEX) != 0; }
  
  nsRect GetDimensions() const { nsRect r = mDimBounds; r.MoveBy(-mPosX, -mPosY); return r; }
  
  

  PRBool HasNonEmptyDirtyRegion() {
    return mDirtyRegion && !mDirtyRegion->IsEmpty();
  }
  nsRegion* GetDirtyRegion() {
    if (!mDirtyRegion) {
      mDirtyRegion = new nsRegion();
      NS_ASSERTION(mDirtyRegion, "Out of memory!");
    }
    return mDirtyRegion;
  }

  void InsertChild(nsView *aChild, nsView *aSibling);
  void RemoveChild(nsView *aChild);

  void SetParent(nsView *aParent) { mParent = aParent; }
  void SetZParent(nsZPlaceholderView *aZParent) { mZParent = aZParent; }
  void SetNextSibling(nsView *aSibling) { mNextSibling = aSibling; }

  PRUint32 GetViewFlags() const { return mVFlags; }
  void SetViewFlags(PRUint32 aFlags) { mVFlags = aFlags; }

  void SetTopMost(PRBool aTopMost) { aTopMost ? mVFlags |= NS_VIEW_FLAG_TOPMOST : mVFlags &= ~NS_VIEW_FLAG_TOPMOST; }
  PRBool IsTopMost() { return((mVFlags & NS_VIEW_FLAG_TOPMOST) != 0); }

  
  
  
  void ConvertToParentCoords(nscoord* aX, nscoord* aY) const { *aX += mPosX; *aY += mPosY; }
  
  
  
  void ConvertFromParentCoords(nscoord* aX, nscoord* aY) const { *aX -= mPosX; *aY -= mPosY; }
  void ResetWidgetBounds(PRBool aRecurse, PRBool aMoveOnly, PRBool aInvalidateChangedSize);
  void SetPositionIgnoringChildWidgets(nscoord aX, nscoord aY);
  nsresult LoadWidget(const nsCID &aClassIID);

  
  
  
  
  void InvalidateHierarchy(nsViewManager *aViewManagerParent);

  virtual ~nsView();

protected:
  
  
  void DoResetWidgetBounds(PRBool aMoveOnly, PRBool aInvalidateChangedSize);
  
  nsZPlaceholderView* mZParent;

  
  nsRect*      mClipRect;
  nsRegion*    mDirtyRegion;
  PRPackedBool mChildRemoved;
};

#endif
