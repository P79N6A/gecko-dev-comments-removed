














































#ifndef mozilla_widget_PuppetWidget_h__
#define mozilla_widget_PuppetWidget_h__

#include "nsBaseWidget.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"

class gfxASurface;

namespace mozilla {
namespace widget {

class PuppetWidget : public nsBaseWidget, public nsSupportsWeakReference
{
  typedef nsBaseWidget Base;

  
  static const size_t kMaxDimension;

public:
  PuppetWidget(PBrowserChild *aTabChild);
  virtual ~PuppetWidget();

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD Create(nsIWidget*        aParent,
                    nsNativeWidget    aNativeParent,
                    const nsIntRect&  aRect,
                    EVENT_CALLBACK    aHandleEventFunction,
                    nsDeviceContext* aContext,
                    nsIAppShell*      aAppShell = nsnull,
                    nsIToolkit*       aToolkit = nsnull,
                    nsWidgetInitData* aInitData = nsnull);

  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              EVENT_CALLBACK   aHandleEventFunction,
              nsDeviceContext *aContext,
              nsIAppShell      *aAppShell = nsnull,
              nsIToolkit       *aToolkit = nsnull,
              nsWidgetInitData *aInitData = nsnull,
              PRBool           aForceUseIWidgetParent = PR_FALSE);

  NS_IMETHOD Destroy();

  NS_IMETHOD Show(PRBool aState);
  NS_IMETHOD IsVisible(PRBool& aState)
  { aState = mVisible; return NS_OK; }

  NS_IMETHOD ConstrainPosition(PRBool   ,
                               PRInt32* aX,
                               PRInt32* aY)
  { *aX = kMaxDimension;  *aY = kMaxDimension;  return NS_OK; }

  
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY)
  { return NS_OK; }

  NS_IMETHOD Resize(PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool  aRepaint);
  NS_IMETHOD Resize(PRInt32 aX,
                    PRInt32 aY,
                    PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool  aRepaint)
  
  { return Resize(aWidth, aHeight, aRepaint); }

  
  
  NS_IMETHOD Enable(PRBool aState)
  { mEnabled = aState;  return NS_OK; }
  NS_IMETHOD IsEnabled(PRBool *aState)
  { *aState = mEnabled;  return NS_OK; }

  NS_IMETHOD SetFocus(PRBool aRaise = PR_FALSE);

  
  virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
  { return NS_OK; }

  NS_IMETHOD Invalidate(const nsIntRect& aRect, PRBool aIsSynchronous);

  NS_IMETHOD Update();

  
  virtual void Scroll(const nsIntPoint& aDelta,
                      const nsTArray<nsIntRect>& aDestRects,
                      const nsTArray<Configuration>& aReconfigureChildren)
  {  }

  
  virtual void* GetNativeData(PRUint32 aDataType)
  { return nsnull; }
  NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent)
  { return NS_ERROR_UNEXPECTED; }

  
  NS_IMETHOD SetTitle(const nsAString& aTitle)
  { return NS_ERROR_UNEXPECTED; }
  
  
  virtual nsIntPoint WidgetToScreenOffset()
  { return nsIntPoint(0, 0); }

  void InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nsnull);

  NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus);

  NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener, nsIMenuRollup* aMenuRollup,
                                 PRBool aDoCapture, PRBool aConsumeRollupEvent)
  { return NS_ERROR_UNEXPECTED; }

  
  
  


  virtual LayerManager*     GetLayerManager(LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                            bool* aAllowRetaining = nsnull);

  virtual gfxASurface*      GetThebesSurface();

  NS_IMETHOD ResetInputState();
  NS_IMETHOD SetIMEOpenState(PRBool aState);
  NS_IMETHOD GetIMEOpenState(PRBool *aState);
  NS_IMETHOD SetInputMode(const IMEContext& aContext);
  NS_IMETHOD GetInputMode(IMEContext& aContext);
  NS_IMETHOD CancelComposition();
  NS_IMETHOD OnIMEFocusChange(PRBool aFocus);
  NS_IMETHOD OnIMETextChange(PRUint32 aOffset, PRUint32 aEnd,
                             PRUint32 aNewEnd);
  NS_IMETHOD OnIMESelectionChange(void);

  
  
  
  
  virtual float GetDPI();

private:
  nsresult DispatchPaintEvent();
  nsresult DispatchResizeEvent();

  void SetChild(PuppetWidget* aChild);

  nsresult IMEEndComposition(PRBool aCancel);

  class PaintTask : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    PaintTask(PuppetWidget* widget) : mWidget(widget) {}
    void Revoke() { mWidget = nsnull; }
  private:
    PuppetWidget* mWidget;
  };

  
  
  
  
  
  
  PBrowserChild *mTabChild;
  
  
  nsRefPtr<PuppetWidget> mChild;
  nsIntRegion mDirtyRegion;
  nsRevocableEventPtr<PaintTask> mPaintTask;
  PRPackedBool mEnabled;
  PRPackedBool mVisible;
  
  
  nsRefPtr<gfxASurface> mSurface;
  
  nsIMEUpdatePreference mIMEPreference;
  PRPackedBool mIMEComposing;
  
  PRUint32 mIMELastReceivedSeqno;
  
  
  
  
  PRUint32 mIMELastBlurSeqno;

  
  float mDPI;
};

}  
}  

#endif  
