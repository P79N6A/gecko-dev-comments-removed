




































#ifndef nsWindow_h
#define nsWindow_h

#include "nsBaseWidget.h"

extern nsIntRect gScreenBounds;

namespace mozilla {
namespace gl {
class GLContext;
}
namespace layers {
class LayersManager;
}
}

namespace android {
class FramebufferNativeWindow;
}

class nsWindow : public nsBaseWidget
{
public:
    nsWindow();
    virtual ~nsWindow();

    static void DoDraw(void);
    static nsEventStatus DispatchInputEvent(nsGUIEvent &aEvent);

    NS_IMETHOD Create(nsIWidget *aParent,
                      void *aNativeParent,
                      const nsIntRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsDeviceContext *aContext,
                      nsWidgetInitData *aInitData);
    NS_IMETHOD Destroy(void);

    NS_IMETHOD Show(bool aState);
    NS_IMETHOD IsVisible(bool & aState);
    NS_IMETHOD ConstrainPosition(bool aAllowSlop,
                                 PRInt32 *aX,
                                 PRInt32 *aY);
    NS_IMETHOD Move(PRInt32 aX,
                    PRInt32 aY);
    NS_IMETHOD Resize(PRInt32 aWidth,
                      PRInt32 aHeight,
                      bool  aRepaint);
    NS_IMETHOD Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      bool aRepaint);
    NS_IMETHOD Enable(bool aState);
    NS_IMETHOD IsEnabled(bool *aState);
    NS_IMETHOD SetFocus(bool aRaise = false);
    NS_IMETHOD ConfigureChildren(const nsTArray<nsIWidget::Configuration>&);
    NS_IMETHOD Invalidate(const nsIntRect &aRect,
                          bool aIsSynchronous);
    NS_IMETHOD Update();
    virtual void* GetNativeData(PRUint32 aDataType);
    NS_IMETHOD SetTitle(const nsAString& aTitle)
    {
        return NS_OK;
    }
    virtual nsIntPoint WidgetToScreenOffset();
    NS_IMETHOD DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus);
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener,
                                   nsIMenuRollup *aMenuRollup,
                                   bool aDoCapture,
                                   bool aConsumeRollupEvent)
    {
        return NS_ERROR_NOT_IMPLEMENTED;
    }
    NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent);

    virtual float GetDPI();
    virtual mozilla::layers::LayerManager*
        GetLayerManager(PLayersChild* aShadowManager = nsnull,
                        LayersBackend aBackendHint = LayerManager::LAYERS_NONE,
                        LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                        bool* aAllowRetaining = nsnull);
    gfxASurface* GetThebesSurface();

protected:
    nsWindow* mParent;
    bool mVisible;

    void BringToTop();
};

#endif 
