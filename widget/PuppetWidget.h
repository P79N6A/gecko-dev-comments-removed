













#ifndef mozilla_widget_PuppetWidget_h__
#define mozilla_widget_PuppetWidget_h__

#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsBaseScreen.h"
#include "nsBaseWidget.h"
#include "nsIScreenManager.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"

class gfxASurface;

namespace mozilla {

namespace dom {
class TabChild;
}

namespace widget {

struct AutoCacheNativeKeyCommands;

class PuppetWidget : public nsBaseWidget
{
  typedef mozilla::dom::TabChild TabChild;
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef nsBaseWidget Base;

  
  static const size_t kMaxDimension;

public:
  explicit PuppetWidget(TabChild* aTabChild);

protected:
  virtual ~PuppetWidget();

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD Create(nsIWidget*        aParent,
                    nsNativeWidget    aNativeParent,
                    const nsIntRect&  aRect,
                    nsWidgetInitData* aInitData = nullptr) override;

  void InitIMEState();

  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              nsWidgetInitData *aInitData = nullptr,
              bool             aForceUseIWidgetParent = false) override;

  NS_IMETHOD Destroy() override;

  NS_IMETHOD Show(bool aState) override;

  virtual bool IsVisible() const override
  { return mVisible; }

  NS_IMETHOD ConstrainPosition(bool     ,
                               int32_t* aX,
                               int32_t* aY) override
  { *aX = kMaxDimension;  *aY = kMaxDimension;  return NS_OK; }

  
  NS_IMETHOD Move(double aX, double aY) override
  { return NS_OK; }

  NS_IMETHOD Resize(double aWidth,
                    double aHeight,
                    bool   aRepaint) override;
  NS_IMETHOD Resize(double aX,
                    double aY,
                    double aWidth,
                    double aHeight,
                    bool   aRepaint) override
  {
    if (mBounds.x != aX || mBounds.y != aY) {
      NotifyWindowMoved(aX, aY);
    }
    mBounds.x = aX;
    mBounds.y = aY;
    return Resize(aWidth, aHeight, aRepaint);
  }

  
  
  NS_IMETHOD Enable(bool aState) override
  { mEnabled = aState;  return NS_OK; }
  virtual bool IsEnabled() const override
  { return mEnabled; }

  NS_IMETHOD SetFocus(bool aRaise = false) override;

  virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations) override;

  NS_IMETHOD Invalidate(const nsIntRect& aRect) override;

  
  virtual void Scroll(const nsIntPoint& aDelta,
                      const nsTArray<nsIntRect>& aDestRects,
                      const nsTArray<Configuration>& aReconfigureChildren)
  {  }

  
  virtual void* GetNativeData(uint32_t aDataType) override;
  NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent) override
  { return NS_ERROR_UNEXPECTED; }

  
  NS_IMETHOD SetTitle(const nsAString& aTitle) override
  { return NS_ERROR_UNEXPECTED; }

  virtual mozilla::LayoutDeviceIntPoint WidgetToScreenOffset() override
  { return LayoutDeviceIntPoint::FromUntyped(GetWindowPosition() + GetChromeDimensions()); }

  
  
  
  virtual nsresult SynthesizeNativeMouseMove(mozilla::LayoutDeviceIntPoint aPoint,
                                             nsIObserver* aObserver) override;

  void InitEvent(WidgetGUIEvent& aEvent, nsIntPoint* aPoint = nullptr);

  NS_IMETHOD DispatchEvent(WidgetGUIEvent* aEvent, nsEventStatus& aStatus) override;
  nsEventStatus DispatchAPZAwareEvent(WidgetInputEvent* aEvent) override;
  nsEventStatus DispatchInputEvent(WidgetInputEvent* aEvent) override;

  NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener,
                                 bool aDoCapture) override
  { return NS_ERROR_UNEXPECTED; }

  NS_IMETHOD_(bool)
  ExecuteNativeKeyBinding(NativeKeyBindingsType aType,
                          const mozilla::WidgetKeyboardEvent& aEvent,
                          DoCommandCallback aCallback,
                          void* aCallbackData) override;

  friend struct AutoCacheNativeKeyCommands;

  
  
  

  
  
  
  
  
  
  virtual nsTransparencyMode GetTransparencyMode() override
  { return eTransparencyTransparent; }

  virtual LayerManager*
  GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                  LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                  LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                  bool* aAllowRetaining = nullptr) override;

  NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                    const InputContextAction& aAction) override;
  NS_IMETHOD_(InputContext) GetInputContext() override;
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() override;

  NS_IMETHOD SetCursor(nsCursor aCursor) override;
  NS_IMETHOD SetCursor(imgIContainer* aCursor,
                       uint32_t aHotspotX, uint32_t aHotspotY) override
  {
    return nsBaseWidget::SetCursor(aCursor, aHotspotX, aHotspotY);
  }

  
  
  
  
  virtual float GetDPI() override;
  virtual double GetDefaultScaleInternal() override;

  virtual bool NeedsPaint() override;

  virtual TabChild* GetOwningTabChild() override { return mTabChild; }
  virtual void ClearBackingScaleCache()
  {
    mDPI = -1;
    mDefaultScale = -1;
  }

  nsIntSize GetScreenDimensions();

  
  nsIntPoint GetChromeDimensions();

  
  nsIntPoint GetWindowPosition();

  NS_IMETHOD GetScreenBounds(nsIntRect &aRect) override;

  NS_IMETHOD StartPluginIME(const mozilla::WidgetKeyboardEvent& aKeyboardEvent,
                            int32_t aPanelX, int32_t aPanelY,
                            nsString& aCommitted) override;

  NS_IMETHOD SetPluginFocused(bool& aFocused) override;

protected:
  bool mEnabled;
  bool mVisible;

  virtual nsresult NotifyIMEInternal(
                     const IMENotification& aIMENotification) override;

private:
  nsresult Paint();

  void SetChild(PuppetWidget* aChild);

  nsresult IMEEndComposition(bool aCancel);
  nsresult NotifyIMEOfFocusChange(bool aFocus);
  nsresult NotifyIMEOfSelectionChange(const IMENotification& aIMENotification);
  nsresult NotifyIMEOfUpdateComposition();
  nsresult NotifyIMEOfTextChange(const IMENotification& aIMENotification);
  nsresult NotifyIMEOfMouseButtonEvent(const IMENotification& aIMENotification);
  nsresult NotifyIMEOfEditorRect();
  nsresult NotifyIMEOfPositionChange();

  bool GetEditorRect(mozilla::LayoutDeviceIntRect& aEditorRect);
  bool GetCompositionRects(uint32_t& aStartOffset,
                           nsTArray<mozilla::LayoutDeviceIntRect>& aRectArray,
                           uint32_t& aTargetCauseOffset);
  bool GetCaretRect(mozilla::LayoutDeviceIntRect& aCaretRect, uint32_t aCaretOffset);
  uint32_t GetCaretOffset();

  class PaintTask : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    explicit PaintTask(PuppetWidget* widget) : mWidget(widget) {}
    void Revoke() { mWidget = nullptr; }
  private:
    PuppetWidget* mWidget;
  };

  
  
  
  
  
  
  TabChild* mTabChild;
  
  
  nsRefPtr<PuppetWidget> mChild;
  nsIntRegion mDirtyRegion;
  nsRevocableEventPtr<PaintTask> mPaintTask;
  
  
  mozilla::RefPtr<DrawTarget> mDrawTarget;
  
  nsIMEUpdatePreference mIMEPreferenceOfParent;
  
  uint32_t mIMELastReceivedSeqno;
  
  
  
  
  uint32_t mIMELastBlurSeqno;
  bool mNeedIMEStateInit;

  
  float mDPI;
  double mDefaultScale;

  
  bool mNativeKeyCommandsValid;
  InfallibleTArray<mozilla::CommandInt> mSingleLineCommands;
  InfallibleTArray<mozilla::CommandInt> mMultiLineCommands;
  InfallibleTArray<mozilla::CommandInt> mRichTextCommands;
};

struct AutoCacheNativeKeyCommands
{
  explicit AutoCacheNativeKeyCommands(PuppetWidget* aWidget)
    : mWidget(aWidget)
  {
    mSavedValid = mWidget->mNativeKeyCommandsValid;
    mSavedSingleLine = mWidget->mSingleLineCommands;
    mSavedMultiLine = mWidget->mMultiLineCommands;
    mSavedRichText = mWidget->mRichTextCommands;
  }

  void Cache(const InfallibleTArray<mozilla::CommandInt>& aSingleLineCommands,
             const InfallibleTArray<mozilla::CommandInt>& aMultiLineCommands,
             const InfallibleTArray<mozilla::CommandInt>& aRichTextCommands)
  {
    mWidget->mNativeKeyCommandsValid = true;
    mWidget->mSingleLineCommands = aSingleLineCommands;
    mWidget->mMultiLineCommands = aMultiLineCommands;
    mWidget->mRichTextCommands = aRichTextCommands;
  }

  void CacheNoCommands()
  {
    mWidget->mNativeKeyCommandsValid = true;
    mWidget->mSingleLineCommands.Clear();
    mWidget->mMultiLineCommands.Clear();
    mWidget->mRichTextCommands.Clear();
  }

  ~AutoCacheNativeKeyCommands()
  {
    mWidget->mNativeKeyCommandsValid = mSavedValid;
    mWidget->mSingleLineCommands = mSavedSingleLine;
    mWidget->mMultiLineCommands = mSavedMultiLine;
    mWidget->mRichTextCommands = mSavedRichText;
  }

private:
  PuppetWidget* mWidget;
  bool mSavedValid;
  InfallibleTArray<mozilla::CommandInt> mSavedSingleLine;
  InfallibleTArray<mozilla::CommandInt> mSavedMultiLine;
  InfallibleTArray<mozilla::CommandInt> mSavedRichText;
};

class PuppetScreen : public nsBaseScreen
{
public:
    explicit PuppetScreen(void* nativeScreen);
    ~PuppetScreen();

    NS_IMETHOD GetId(uint32_t* aId) override;
    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight) override;
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight) override;
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth) override;
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth) override;
    NS_IMETHOD GetRotation(uint32_t* aRotation) override;
    NS_IMETHOD SetRotation(uint32_t  aRotation) override;
};

class PuppetScreenManager final : public nsIScreenManager
{
    ~PuppetScreenManager();

public:
    PuppetScreenManager();

    NS_DECL_ISUPPORTS
    NS_DECL_NSISCREENMANAGER

protected:
    nsCOMPtr<nsIScreen> mOneScreen;
};

}  
}  

#endif  
