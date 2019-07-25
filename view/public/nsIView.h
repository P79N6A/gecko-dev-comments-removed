




































#ifndef nsIView_h___
#define nsIView_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsNativeWidget.h"
#include "nsIWidget.h"
#include "nsWidgetInitData.h"
#include "nsIFrame.h"

class nsIViewManager;
class nsViewManager;
class nsView;
class nsWeakView;
class nsIWidget;





enum nsViewVisibility {
  nsViewVisibility_kHide = 0,
  nsViewVisibility_kShow = 1
};

#define NS_IVIEW_IID    \
  { 0xda62efbf, 0x0711, 0x4b79, \
    { 0x87, 0x85, 0x9e, 0xec, 0xed, 0xf5, 0xb0, 0x32 } }




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

  








  void SetInvalidationDimensions(const nsRect* aRect);

  

















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

  








  nsresult CreateWidget(nsWidgetInitData *aWidgetInitData = nsnull,
                        bool aEnableDragDrop = true,
                        bool aResetVisibility = true);

  




  nsresult CreateWidgetForParent(nsIWidget* aParentWidget,
                                 nsWidgetInitData *aWidgetInitData = nsnull,
                                 bool aEnableDragDrop = true,
                                 bool aResetVisibility = true);

  






  nsresult CreateWidgetForPopup(nsWidgetInitData *aWidgetInitData,
                                nsIWidget* aParentWidget = nsnull,
                                bool aEnableDragDrop = true,
                                bool aResetVisibility = true);

  




  void DestroyWidget();

  










  nsresult AttachToTopLevelWidget(nsIWidget* aWidget);
  nsresult DetachFromTopLevelWidget();

  



  bool IsAttachedToTopLevel() const { return mWidgetIsTopLevel; }

  






  nsIWidget* GetWidget() const { return mWindow; }

  


  bool HasWidget() const { return mWindow != nsnull; }

  




  EVENT_CALLBACK AttachWidgetEventHandler(nsIWidget* aWidget);
  


  void DetachWidgetEventHandler(nsIWidget* aWidget);

#ifdef DEBUG
  





  virtual void List(FILE* out, PRInt32 aIndent = 0) const;
#endif 

  


  bool IsRoot() const;

  virtual bool ExternalIsRoot() const;

  void SetDeletionObserver(nsWeakView* aDeletionObserver);

  nsIntRect CalcWidgetBounds(nsWindowType aType);

  bool IsEffectivelyVisible();

  
  
  
  
  nsPoint ViewToWidgetOffset() const { return mViewToWidgetOffset; }

protected:
  friend class nsWeakView;
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
  nsWeakView*       mDeletionObserver;
  bool              mWidgetIsTopLevel;

  virtual ~nsIView() {}

private:
  nsView* Impl();
  const nsView* Impl() const;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIView, NS_IVIEW_IID)


class nsWeakView
{
public:
  nsWeakView(nsIView* aView) : mPrev(nsnull), mView(aView)
  {
    if (mView) {
      mView->SetDeletionObserver(this);
    }
  }

  ~nsWeakView()
  {
    if (mView) {
      NS_ASSERTION(mView->mDeletionObserver == this,
                   "nsWeakViews deleted in wrong order!");
      
      mView->SetDeletionObserver(nsnull);
      
      mView->SetDeletionObserver(mPrev);
    }
  }

  bool IsAlive() { return !!mView; }

  nsIView* GetView() { return mView; }

  void SetPrevious(nsWeakView* aWeakView) { mPrev = aWeakView; }

  void Clear()
  {
    if (mPrev) {
      mPrev->Clear();
    }
    mView = nsnull;
  }
private:
  static void* operator new(size_t) CPP_THROW_NEW { return 0; }
  static void operator delete(void*, size_t) {}
  nsWeakView* mPrev;
  nsIView*    mView;
};

#endif
