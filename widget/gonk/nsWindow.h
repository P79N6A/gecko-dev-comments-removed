














#ifndef nsWindow_h
#define nsWindow_h

#include "InputData.h"
#include "nsBaseWidget.h"
#include "nsRegion.h"
#include "nsIIdleServiceInternal.h"
#include "Units.h"

extern nsIntRect gScreenBounds;

namespace mozilla {
namespace gl {
class GLContext;
}
namespace layers {
class LayersManager;
}
}

class ANativeWindowBuffer;

namespace android {
class FramebufferNativeWindow;
}

namespace widget {
struct InputContext;
struct InputContextAction;
}

class nsWindow : public nsBaseWidget
{
public:
    nsWindow();

    NS_DECL_ISUPPORTS_INHERITED

    static void DoDraw(void);
    static nsEventStatus DispatchInputEvent(mozilla::WidgetGUIEvent& aEvent);
    static void DispatchTouchInput(mozilla::MultiTouchInput& aInput);

    NS_IMETHOD Create(nsIWidget *aParent,
                      void *aNativeParent,
                      const nsIntRect &aRect,
                      nsWidgetInitData *aInitData);
    NS_IMETHOD Destroy(void);

    NS_IMETHOD Show(bool aState);
    virtual bool IsVisible() const;
    NS_IMETHOD ConstrainPosition(bool aAllowSlop,
                                 int32_t *aX,
                                 int32_t *aY);
    NS_IMETHOD Move(double aX,
                    double aY);
    NS_IMETHOD Resize(double aWidth,
                      double aHeight,
                      bool  aRepaint);
    NS_IMETHOD Resize(double aX,
                      double aY,
                      double aWidth,
                      double aHeight,
                      bool aRepaint);
    NS_IMETHOD Enable(bool aState);
    virtual bool IsEnabled() const;
    NS_IMETHOD SetFocus(bool aRaise = false);
    NS_IMETHOD ConfigureChildren(const nsTArray<nsIWidget::Configuration>&);
    NS_IMETHOD Invalidate(const nsIntRect &aRect);
    virtual void* GetNativeData(uint32_t aDataType);
    NS_IMETHOD SetTitle(const nsAString& aTitle)
    {
        return NS_OK;
    }
    virtual mozilla::LayoutDeviceIntPoint WidgetToScreenOffset();
    void DispatchTouchInputViaAPZ(mozilla::MultiTouchInput& aInput);
    void DispatchTouchEventForAPZ(const mozilla::MultiTouchInput& aInput,
                                  const ScrollableLayerGuid& aGuid,
                                  const uint64_t aInputBlockId,
                                  nsEventStatus aApzResponse);
    NS_IMETHOD DispatchEvent(mozilla::WidgetGUIEvent* aEvent,
                             nsEventStatus& aStatus);
    virtual nsresult SynthesizeNativeTouchPoint(uint32_t aPointerId,
                                                TouchPointerState aPointerState,
                                                nsIntPoint aPointerScreenPoint,
                                                double aPointerPressure,
                                                uint32_t aPointerOrientation) override;

    NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener,
                                   bool aDoCapture)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent);

    NS_IMETHOD MakeFullScreen(bool aFullScreen, nsIScreen* aTargetScreen = nullptr) ;

    virtual mozilla::TemporaryRef<mozilla::gfx::DrawTarget>
        StartRemoteDrawing() override;
    virtual void EndRemoteDrawing() override;

    virtual float GetDPI();
    virtual double GetDefaultScaleInternal();
    virtual mozilla::layers::LayerManager*
        GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                        LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                        LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                        bool* aAllowRetaining = nullptr);

    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction);
    NS_IMETHOD_(InputContext) GetInputContext();

    virtual uint32_t GetGLFrameBufferFormat() override;

    virtual nsIntRect GetNaturalBounds() override;
    virtual bool NeedsPaint();

    virtual Composer2D* GetComposer2D() override;

protected:
    nsWindow* mParent;
    bool mVisible;
    InputContext mInputContext;
    nsCOMPtr<nsIIdleServiceInternal> mIdleService;
    
    
    
    mozilla::RefPtr<mozilla::gfx::DrawTarget> mFramebufferTarget;
    ANativeWindowBuffer* mFramebuffer;
    
    
    
    
    
    
    
    
    mozilla::RefPtr<mozilla::gfx::DrawTarget> mBackBuffer;

    virtual ~nsWindow();

    void BringToTop();

    
    
    void UserActivity();

private:
    
    
    nsAutoPtr<mozilla::MultiTouchInput> mSynthesizedTouchInput;
};

#endif 
