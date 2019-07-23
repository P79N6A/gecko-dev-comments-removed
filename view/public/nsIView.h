




































#ifndef nsIView_h___
#define nsIView_h___

#include "nsCoord.h"
#include "nsRect.h"
#include "nsPoint.h"
#include "nsIWidget.h"

class nsIViewManager;
class nsIScrollableView;
class nsViewManager;
class nsView;





enum nsViewVisibility {
  nsViewVisibility_kHide = 0,
  nsViewVisibility_kShow = 1
};



#define NS_IVIEW_IID    \
{ 0x6610ae89, 0x3909, 0x422f, \
{ 0xa2, 0x27, 0xed, 0xe6, 0x7b, 0x97, 0xbc, 0xd1 } }


#define NS_VIEW_FLAGS_PUBLIC              0x00FF


#define NS_VIEW_FLAGS_PRIVATE             0xFF00




#define NS_VIEW_FLAG_AUTO_ZINDEX          0x0004


#define NS_VIEW_FLAG_FLOATING             0x0008




#define NS_VIEW_FLAG_TOPMOST              0x0010

struct nsViewZIndex {
  PRBool mIsAuto;
  PRInt32 mZIndex;
  PRBool mIsTopmost;
  
  nsViewZIndex(PRBool aIsAuto, PRInt32 aZIndex, PRBool aIsTopmost)
    : mIsAuto(aIsAuto), mZIndex(aZIndex), mIsTopmost(aIsTopmost) {}
};
















class nsIView
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IVIEW_IID)

  



  virtual nsIScrollableView* ToScrollableView() { return nsnull; }

  




  static nsIView* GetViewFor(nsIWidget* aWidget);

  





  nsIViewManager* GetViewManager() const
  { return reinterpret_cast<nsIViewManager*>(mViewManager); }

  









  void Destroy();

  






  nsPoint GetPosition() const {
    
    
    NS_ASSERTION(!ExternalIsRoot() || (mPosX == 0 && mPosY == 0),
                 "root views should always have explicit position of (0,0)");
    return nsPoint(mPosX, mPosY);
  }
  
  






  nsRect GetBounds() const { return mDimBounds; }

  












  nsPoint GetOffsetTo(const nsIView* aOther) const;

  




  nsIntPoint GetScreenPosition() const;
  
  



  nsViewVisibility GetVisibility() const { return mVis; }

  








  nsViewZIndex GetZIndex() const { return nsViewZIndex((mVFlags & NS_VIEW_FLAG_AUTO_ZINDEX) != 0,
                                                       mZIndex,
                                                       (mVFlags & NS_VIEW_FLAG_TOPMOST) != 0); }

  







  PRBool GetFloating() const { return (mVFlags & NS_VIEW_FLAG_FLOATING) != 0; }

  



  nsIView* GetParent() const { return reinterpret_cast<nsIView*>(mParent); }

  



  nsIView* GetFirstChild() const { return reinterpret_cast<nsIView*>(mFirstChild); }

  



  nsIView* GetNextSibling() const { return reinterpret_cast<nsIView*>(mNextSibling); }

  



  void SetClientData(void *aData) { mClientData = aData; }

  



  void* GetClientData() const { return mClientData; }

  










  virtual nsIWidget* GetNearestWidget(nsPoint* aOffset) const;

  














  nsresult CreateWidget(const nsIID &aWindowIID,
                        nsWidgetInitData *aWidgetInitData = nsnull,
                        nsNativeWidget aNative = nsnull,
                        PRBool aEnableDragDrop = PR_TRUE,
                        PRBool aResetVisibility = PR_TRUE,
                        nsContentType aWindowType = eContentTypeInherit);

  






  nsIWidget* GetWidget() const { return mWindow; }

  


  PRBool HasWidget() const { return mWindow != nsnull; }

  



  void DisownWidget() {
    mWidgetDisowned = PR_TRUE;
  }

#ifdef DEBUG
  





  virtual void List(FILE* out, PRInt32 aIndent = 0) const;
#endif 

  


  PRBool IsRoot() const;

  virtual PRBool ExternalIsRoot() const;

protected:
  nsViewManager     *mViewManager;
  nsView            *mParent;
  nsIWidget         *mWindow;
  nsView            *mNextSibling;
  nsView            *mFirstChild;
  void              *mClientData;
  PRInt32           mZIndex;
  nsViewVisibility  mVis;
  nscoord           mPosX, mPosY;
  nsRect            mDimBounds; 
  float             mOpacity;
  PRUint32          mVFlags;
  PRBool            mWidgetDisowned;

  virtual ~nsIView() {}
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIView, NS_IVIEW_IID)

#endif
