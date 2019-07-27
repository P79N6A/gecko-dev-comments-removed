













#ifndef mozilla_widget_PuppetWidget_h__
#define mozilla_widget_PuppetWidget_h__

#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsBaseScreen.h"
#include "nsBaseWidget.h"
#include "nsIScreenManager.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "mozilla/Attributes.h"
#include "mozilla/EventForwards.h"

class gfxASurface;

namespace mozilla {

namespace dom {
class TabChild;
}

namespace widget {

struct AutoCacheNativeKeyCommands;

class PuppetWidget : public nsBaseWidget,
                     public nsSupportsWeakReference
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
                    nsDeviceContext*  aContext,
                    nsWidgetInitData* aInitData = nullptr) MOZ_OVERRIDE;

  void InitIMEState();

  virtual already_AddRefed<nsIWidget>
  CreateChild(const nsIntRect  &aRect,
              nsDeviceContext  *aContext,
              nsWidgetInitData *aInitData = nullptr,
              bool             aForceUseIWidgetParent = false) MOZ_OVERRIDE;

  NS_IMETHOD Destroy() MOZ_OVERRIDE;

  NS_IMETHOD Show(bool aState) MOZ_OVERRIDE;

  virtual bool IsVisible() const MOZ_OVERRIDE
  { return mVisible; }

  NS_IMETHOD ConstrainPosition(bool     ,
                               int32_t* aX,
                               int32_t* aY) MOZ_OVERRIDE
  { *aX = kMaxDimension;  *aY = kMaxDimension;  return NS_OK; }

  
  NS_IMETHOD Move(double aX, double aY) MOZ_OVERRIDE
  { return NS_OK; }

  NS_IMETHOD Resize(double aWidth,
                    double aHeight,
                    bool   aRepaint) MOZ_OVERRIDE;
  NS_IMETHOD Resize(double aX,
                    double aY,
                    double aWidth,
                    double aHeight,
                    bool   aRepaint) MOZ_OVERRIDE
  
  { return Resize(aWidth, aHeight, aRepaint); }

  
  
  NS_IMETHOD Enable(bool aState) MOZ_OVERRIDE
  { mEnabled = aState;  return NS_OK; }
  virtual bool IsEnabled() const MOZ_OVERRIDE
  { return mEnabled; }

  NS_IMETHOD SetFocus(bool aRaise = false) MOZ_OVERRIDE;

  virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations) MOZ_OVERRIDE;

  NS_IMETHOD Invalidate(const nsIntRect& aRect) MOZ_OVERRIDE;

  
  virtual void Scroll(const nsIntPoint& aDelta,
                      const nsTArray<nsIntRect>& aDestRects,
                      const nsTArray<Configuration>& aReconfigureChildren)
  {  }

  
  virtual void* GetNativeData(uint32_t aDataType) MOZ_OVERRIDE;
  NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

  
  NS_IMETHOD SetTitle(const nsAString& aTitle) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }
  
  
  virtual mozilla::LayoutDeviceIntPoint WidgetToScreenOffset() MOZ_OVERRIDE
  { return mozilla::LayoutDeviceIntPoint(0, 0); }

  void InitEvent(WidgetGUIEvent& aEvent, nsIntPoint* aPoint = nullptr);

  NS_IMETHOD DispatchEvent(WidgetGUIEvent* aEvent, nsEventStatus& aStatus) MOZ_OVERRIDE;

  NS_IMETHOD CaptureRollupEvents(nsIRollupListener* aListener,
                                 bool aDoCapture) MOZ_OVERRIDE
  { return NS_ERROR_UNEXPECTED; }

  NS_IMETHOD_(bool)
  ExecuteNativeKeyBinding(NativeKeyBindingsType aType,
                          const mozilla::WidgetKeyboardEvent& aEvent,
                          DoCommandCallback aCallback,
                          void* aCallbackData) MOZ_OVERRIDE;

  friend struct AutoCacheNativeKeyCommands;

  
  
  

  
  
  
  
  
  
  virtual nsTransparencyMode GetTransparencyMode() MOZ_OVERRIDE
  { return eTransparencyTransparent; }

  virtual LayerManager*
  GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                  LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                  LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                  bool* aAllowRetaining = nullptr) MOZ_OVERRIDE;

  NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                    const InputContextAction& aAction) MOZ_OVERRIDE;
  NS_IMETHOD_(InputContext) GetInputContext() MOZ_OVERRIDE;
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() MOZ_OVERRIDE;

  NS_IMETHOD SetCursor(nsCursor aCursor) MOZ_OVERRIDE;
  NS_IMETHOD SetCursor(imgIContainer* aCursor,
                       uint32_t aHotspotX, uint32_t aHotspotY) MOZ_OVERRIDE
  {
    return nsBaseWidget::SetCursor(aCursor, aHotspotX, aHotspotY);
  }

  
  
  
  
  virtual float GetDPI() MOZ_OVERRIDE;
  virtual double GetDefaultScaleInternal() MOZ_OVERRIDE;

  virtual bool NeedsPaint() MOZ_OVERRIDE;

  virtual TabChild* GetOwningTabChild() MOZ_OVERRIDE { return mTabChild; }
  virtual void ClearBackingScaleCache()
  {
    mDPI = -1;
    mDefaultScale = -1;
  }

  nsIntSize GetScreenDimensions();

  
  nsIntPoint GetChromeDimensions();

  
  nsIntPoint GetWindowPosition();

protected:
  bool mEnabled;
  bool mVisible;

  virtual nsresult NotifyIMEInternal(
                     const IMENotification& aIMENotification) MOZ_OVERRIDE;

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
  bool mIMEComposing;
  
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

    NS_IMETHOD GetId(uint32_t* aId) MOZ_OVERRIDE;
    NS_IMETHOD GetRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight) MOZ_OVERRIDE;
    NS_IMETHOD GetAvailRect(int32_t* aLeft, int32_t* aTop, int32_t* aWidth, int32_t* aHeight) MOZ_OVERRIDE;
    NS_IMETHOD GetPixelDepth(int32_t* aPixelDepth) MOZ_OVERRIDE;
    NS_IMETHOD GetColorDepth(int32_t* aColorDepth) MOZ_OVERRIDE;
    NS_IMETHOD GetRotation(uint32_t* aRotation) MOZ_OVERRIDE;
    NS_IMETHOD SetRotation(uint32_t  aRotation) MOZ_OVERRIDE;
};

class PuppetScreenManager MOZ_FINAL : public nsIScreenManager
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
