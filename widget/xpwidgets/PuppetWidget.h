













#ifndef mozilla_widget_PuppetWidget_h__
#define mozilla_widget_PuppetWidget_h__

#include "nsBaseScreen.h"
#include "nsBaseWidget.h"
#include "nsIScreenManager.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"

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
                    nsDeviceContext*  aContext,
                    nsWidgetInitData* aInitData = nullptr);

  void InitIMEState();

  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              nsDeviceContext  *aContext,
              nsWidgetInitData *aInitData = nullptr,
              bool             aForceUseIWidgetParent = false);

  NS_IMETHOD Destroy();

  NS_IMETHOD Show(bool aState);

  virtual bool IsVisible() const
  { return mVisible; }

  NS_IMETHOD ConstrainPosition(bool     ,
                               int32_t* aX,
                               int32_t* aY)
  { *aX = kMaxDimension;  *aY = kMaxDimension;  return NS_OK; }

  
  NS_IMETHOD Move(double aX, double aY)
  { return NS_OK; }

  NS_IMETHOD Resize(double aWidth,
                    double aHeight,
                    bool   aRepaint);
  NS_IMETHOD Resize(double aX,
                    double aY,
                    double aWidth,
                    double aHeight,
                    bool   aRepaint)
  
  { return Resize(aWidth, aHeight, aRepaint); }

  
  
  NS_IMETHOD Enable(bool aState)
  { mEnabled = aState;  return NS_OK; }
  virtual bool IsEnabled() const
  { return mEnabled; }

  NS_IMETHOD SetFocus(bool aRaise = false);

  
  virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations)
  { return NS_OK; }

  NS_IMETHOD Invalidate(const nsIntRect& aRect);

  
  virtual void Scroll(const nsIntPoint& aDelta,
                      const nsTArray<nsIntRect>& aDestRects,
                      const nsTArray<Configuration>& aReconfigureChildren)
  {  }

  
  virtual void* GetNativeData(uint32_t aDataType);
  NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent)
  { return NS_ERROR_UNEXPECTED; }

  
  NS_IMETHOD SetTitle(const nsAString& aTitle)
  { return NS_ERROR_UNEXPECTED; }
  
  
  virtual nsIntPoint WidgetToScreenOffset()
  { return nsIntPoint(0, 0); }

  void InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = nullptr);

  NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus& aStatus);

  NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener,
                                 bool aDoCapture)
  { return NS_ERROR_UNEXPECTED; }

  
  
  

  
  
  
  
  
  
  virtual nsTransparencyMode GetTransparencyMode() MOZ_OVERRIDE
  { return eTransparencyTransparent; }

  virtual LayerManager*
  GetLayerManager(PLayersChild* aShadowManager = nullptr,
                  LayersBackend aBackendHint = mozilla::layers::LAYERS_NONE,
                  LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                  bool* aAllowRetaining = nullptr);
  virtual gfxASurface*      GetThebesSurface();

  NS_IMETHOD NotifyIME(NotificationToIME aNotification) MOZ_OVERRIDE;
  NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                    const InputContextAction& aAction);
  NS_IMETHOD_(InputContext) GetInputContext();
  NS_IMETHOD NotifyIMEOfTextChange(uint32_t aOffset, uint32_t aEnd,
                                   uint32_t aNewEnd) MOZ_OVERRIDE;
  virtual nsIMEUpdatePreference GetIMEUpdatePreference();

  NS_IMETHOD SetCursor(nsCursor aCursor);
  NS_IMETHOD SetCursor(imgIContainer* aCursor,
                       uint32_t aHotspotX, uint32_t aHotspotY)
  {
    return nsBaseWidget::SetCursor(aCursor, aHotspotX, aHotspotY);
  }

  
  
  
  
  virtual float GetDPI();

  virtual bool NeedsPaint() MOZ_OVERRIDE;

  virtual TabChild* GetOwningTabChild() MOZ_OVERRIDE { return mTabChild; }

private:
  nsresult Paint();

  void SetChild(PuppetWidget* aChild);

  nsresult IMEEndComposition(bool aCancel);
  nsresult NotifyIMEOfFocusChange(bool aFocus);
  nsresult NotifyIMEOfSelectionChange();

  class PaintTask : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    PaintTask(PuppetWidget* widget) : mWidget(widget) {}
    void Revoke() { mWidget = nullptr; }
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
  
  uint32_t mIMELastReceivedSeqno;
  
  
  
  
  uint32_t mIMELastBlurSeqno;
  bool mNeedIMEStateInit;

  
  float mDPI;
};

class PuppetScreen : public nsBaseScreen
{
public:
    PuppetScreen(void* nativeScreen);
    ~PuppetScreen();

    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight) MOZ_OVERRIDE;
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight) MOZ_OVERRIDE;
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth) MOZ_OVERRIDE;
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth) MOZ_OVERRIDE;
    NS_IMETHOD GetRotation(uint32_t* aRotation) MOZ_OVERRIDE;
    NS_IMETHOD SetRotation(uint32_t  aRotation) MOZ_OVERRIDE;
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
