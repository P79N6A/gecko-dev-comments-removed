




#ifndef nsIView_h___
#define nsIView_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsNativeWidget.h"
#include "nsIWidget.h"
#include "nsWidgetInitData.h"

class nsIViewManager;
class nsViewManager;
class nsView;
class nsIWidget;
class nsIFrame;





enum nsViewVisibility {
  nsViewVisibility_kHide = 0,
  nsViewVisibility_kShow = 1
};

#define NS_IVIEW_IID    \
  { 0xa4577c1d, 0xbc80, 0x444c, \
    { 0xb0, 0x9d, 0x5b, 0xef, 0x94, 0x7c, 0x43, 0x31 } }




#define NS_VIEW_FLAG_AUTO_ZINDEX          0x0004


#define NS_VIEW_FLAG_FLOATING             0x0008




#define NS_VIEW_FLAG_TOPMOST              0x0010
















class nsIView
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEW_IID)

  




  static nsIView* GetViewFor(nsIWidget* aWidget);

  





  nsIViewManager* GetViewManager() const
  { return reinterpret_cast<nsIViewManager*>(mViewManager); }

  









  void Destroy();

  







  nsPoint GetPosition() const {
    
    
    NS_ASSERTION(!ExternalIsRoot() || (mPosX == 0 && mPosY == 0),
                 "root views should always have explicit position of (0,0)");
    return nsPoint(mPosX, mPosY);
  }

  



  virtual void SetPosition(nscoord aX, nscoord aY) = 0;
  
  







  nsRect GetBounds() const { return mDimBounds; }

  



  nsRect GetDimensions() const {
    nsRect r = mDimBounds; r.MoveBy(-mPosX, -mPosY); return r;
  }

  

















  nsPoint GetOffsetTo(const nsIView* aOther) const;

  






  nsPoint GetOffsetToWidget(nsIWidget* aWidget) const;

  




  nsPoint ConvertFromParentCoords(nsPoint aPt) const;

  



  nsViewVisibility GetVisibility() const { return mVis; }

  







  bool GetFloating() const { return (mVFlags & NS_VIEW_FLAG_FLOATING) != 0; }

  



  nsIView* GetParent() const { return reinterpret_cast<nsIView*>(mParent); }

  



  nsIView* GetFirstChild() const { return reinterpret_cast<nsIView*>(mFirstChild); }

  



  nsIView* GetNextSibling() const { return reinterpret_cast<nsIView*>(mNextSibling); }
  void SetNextSibling(nsIView *aSibling) {
    mNextSibling = reinterpret_cast<nsView*>(aSibling);
  }

  


  void SetFrame(nsIFrame* aRootFrame) { mFrame = aRootFrame; }

  


  nsIFrame* GetFrame() const { return mFrame; }

  











  virtual nsIWidget* GetNearestWidget(nsPoint* aOffset) const;

  








  nsresult CreateWidget(nsWidgetInitData *aWidgetInitData = nullptr,
                        bool aEnableDragDrop = true,
                        bool aResetVisibility = true);

  




  nsresult CreateWidgetForParent(nsIWidget* aParentWidget,
                                 nsWidgetInitData *aWidgetInitData = nullptr,
                                 bool aEnableDragDrop = true,
                                 bool aResetVisibility = true);

  






  nsresult CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                nsIWidget* aParentWidget = nullptr,
                                bool aEnableDragDrop = true,
                                bool aResetVisibility = true);

  




  void DestroyWidget();

  










  nsresult AttachToTopLevelWidget(nsIWidget* aWidget);
  nsresult DetachFromTopLevelWidget();

  



  bool IsAttachedToTopLevel() const { return mWidgetIsTopLevel; }

  






  nsIWidget* GetWidget() const { return mWindow; }

  


  bool HasWidget() const { return mWindow != nullptr; }

  




  void AttachWidgetEventHandler(nsIWidget* aWidget);
  


  void DetachWidgetEventHandler(nsIWidget* aWidget);

#ifdef DEBUG
  





  virtual void List(FILE* out, PRInt32 aIndent = 0) const;
#endif 

  


  bool IsRoot() const;

  virtual bool ExternalIsRoot() const;

  nsIntRect CalcWidgetBounds(nsWindowType aType);

  bool IsEffectivelyVisible();

  
  
  
  
  nsPoint ViewToWidgetOffset() const { return mViewToWidgetOffset; }

protected:
  nsViewManager     *mViewManager;
  nsView            *mParent;
  nsIWidget         *mWindow;
  nsView            *mNextSibling;
  nsView            *mFirstChild;
  nsIFrame          *mFrame;
  PRInt32           mZIndex;
  nsViewVisibility  mVis;
  
  nscoord           mPosX, mPosY;
  
  nsRect            mDimBounds;
  
  nsPoint           mViewToWidgetOffset;
  float             mOpacity;
  PRUint32          mVFlags;
  bool              mWidgetIsTopLevel;

  virtual ~nsIView() {}

private:
  nsView* Impl();
  const nsView* Impl() const;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIView, NS_IVIEW_IID)

#endif
