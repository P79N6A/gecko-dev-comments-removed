













#ifndef mozilla_widget_PuppetWidget_h__
#define mozilla_widget_PuppetWidget_h__

#include "nsBaseScreen.h"
#include "nsBaseWidget.h"
#include "nsIScreenManager.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"
#include "LayersBackend.h"

class gfxASurface;

namespace mozilla {

namespace dom {
class TabChild;
}

namespace widget {

class PuppetWidget : public nsBaseWidget, public nsSupportsWeakReference
{
  typedef mozilla::dom::TabChild TabChild;
  typedef nsBaseWidget Base;

  
  static const size_t kMaxDimension;

public:
  PuppetWidget(TabChild* aTabChild);
  virtual ~PuppetWidget();

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD Create(nsIWidget*        aParent,
                    nsNativeWidget    aNativeParent,
                    const nsIntRect&  aRect,
                    EVENT_CALLBACK    aHandleEventFunction,
                    nsDeviceContext*  aContext,
                    nsWidgetInitData* aInitData = nsnull);

  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              EVENT_CALLBACK   aHandleEventFunction,
              nsDeviceContext  *aContext,
              nsWidgetInitData *aInitData = nsnull,
              bool             aForceUseIWidgetParent = false);

  NS_IMETHOD Destroy();

  NS_IMETHOD Show(bool aState);

  virtual bool IsVisible() const
  { return mVisible; }

  NS_IMETHOD ConstrainPosition(bool     ,
                               PRInt32* aX,
                               PRInt32* aY)
  { *aX = kMaxDimension;  *aY = kMaxDimension;  return NS_OK; }

  
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY)
  { return NS_OK; }

  NS_IMETHOD Resize(PRInt32 aWidth,
                    PRInt32 aHeight,
                    bool    aRepaint);
  NS_IMETHOD Resize(PRInt32 aX,
                    PRInt32 aY,
                    PRInt32 aWidth,
                    PRInt32 aHeight,
                    bool    aRepaint)
  
  { return Resize(aWidth, aHeight, aRepaint); }

  
  
  NS_IMETHOD Enable(bool aState)
  { mEnabled = aState;  return NS_OK; }
  NS_IMETHOD IsEnabled(bool *aState)
  { *aState = mEnabled;  return NS_OK; }

  NS_IMETHOD SetFocus(bool aRaise = false);

  
  virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
  { return NS_OK; }

  NS_IMETHOD Invalidate(const nsIntRect& aRect);

  
  virtual void Scroll(const nsIntPoint& aDelta,
                      const nsTArray<nsIntRect>& aDestRects,
                      const nsTArray<Configuration>& aReconfigureChildren)
  {  }

  
  virtual void* GetNativeData(PRUint32 aDataType);
  NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent)
  { return NS_ERROR_UNEXPECTED; }

  
  NS_IMETHOD SetTitle(const nsAString& aTitle)
  { return NS_ERROR_UNEXPECTED; }
  
  
  virtual nsIntPoint WidgetToScreenOffset()
  { return nsIntPoint(0, 0); }

  void InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nsnull);

  NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus);

  NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener,
                                 bool aDoCapture, bool aConsumeRollupEvent)
  { return NS_ERROR_UNEXPECTED; }

  
  
  


  virtual LayerManager*
  GetLayerManager(PLayersChild* aShadowManager = nsnull,
                  LayersBackend aBackendHint = mozilla::layers::LAYERS_NONE,
                  LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                  bool* aAllowRetaining = nsnull);

  virtual gfxASurface*      GetThebesSurface();

  NS_IMETHOD ResetInputState();
  NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                    const InputContextAction& aAction);
  NS_IMETHOD_(InputContext) GetInputContext();
  NS_IMETHOD CancelComposition();
  NS_IMETHOD OnIMEFocusChange(bool aFocus);
  NS_IMETHOD OnIMETextChange(PRUint32 aOffset, PRUint32 aEnd,
                             PRUint32 aNewEnd);
  NS_IMETHOD OnIMESelectionChange(void);

  NS_IMETHOD SetCursor(nsCursor aCursor);
  NS_IMETHOD SetCursor(imgIContainer* aCursor,
                       PRUint32 aHotspotX, PRUint32 aHotspotY)
  {
    return nsBaseWidget::SetCursor(aCursor, aHotspotX, aHotspotY);
  }

  
  
  
  
  virtual float GetDPI();

private:
  nsresult DispatchPaintEvent();
  nsresult DispatchResizeEvent();

  void SetChild(PuppetWidget* aChild);

  nsresult IMEEndComposition(bool aCancel);

  class PaintTask : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    PaintTask(PuppetWidget* widget) : mWidget(widget) {}
    void Revoke() { mWidget = nsnull; }
  private:
    PuppetWidget* mWidget;
  };

  
  
  
  
  
  
  TabChild* mTabChild;
  
  
  nsRefPtr<PuppetWidget> mChild;
  nsIntRegion mDirtyRegion;
  nsRevocableEventPtr<PaintTask> mPaintTask;
  bool mEnabled;
  bool mVisible;
  
  
  nsRefPtr<gfxASurface> mSurface;
  
  nsIMEUpdatePreference mIMEPreference;
  bool mIMEComposing;
  
  PRUint32 mIMELastReceivedSeqno;
  
  
  
  
  PRUint32 mIMELastBlurSeqno;

  
  float mDPI;
};

class PuppetScreen : public nsBaseScreen
{
public:
    PuppetScreen(void* nativeScreen);
    ~PuppetScreen();

    NS_IMETHOD GetRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight) MOZ_OVERRIDE;
    NS_IMETHOD GetAvailRect(PRInt32* aLeft, PRInt32* aTop, PRInt32* aWidth, PRInt32* aHeight) MOZ_OVERRIDE;
    NS_IMETHOD GetPixelDepth(PRInt32* aPixelDepth) MOZ_OVERRIDE;
    NS_IMETHOD GetColorDepth(PRInt32* aColorDepth) MOZ_OVERRIDE;
    NS_IMETHOD GetRotation(PRUint32* aRotation) MOZ_OVERRIDE;
    NS_IMETHOD SetRotation(PRUint32  aRotation) MOZ_OVERRIDE;
};

class PuppetScreenManager MOZ_FINAL : public nsIScreenManager
{
public:
    PuppetScreenManager();
    ~PuppetScreenManager();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREENMANAGER

protected:
    nsCOMPtr<nsIScreen> mOneScreen;
};

}  
}  

#endif  
