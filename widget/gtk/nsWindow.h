






#ifndef __nsWindow_h__
#define __nsWindow_h__

#include "mozilla/ipc/SharedMemorySysV.h"

#include "nsAutoPtr.h"

#include "mozcontainer.h"

#include "nsIDragService.h"
#include "nsITimer.h"
#include "nsGkAtoms.h"

#include "nsBaseWidget.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 

#ifdef ACCESSIBILITY
#include "mozilla/a11y/Accessible.h"
#endif
#include "mozilla/EventForwards.h"

#include "nsGtkIMModule.h"

#undef LOG
#ifdef MOZ_LOGGING

#include "prlog.h"
#include "nsTArray.h"
#include "Units.h"

extern PRLogModuleInfo *gWidgetLog;
extern PRLogModuleInfo *gWidgetFocusLog;
extern PRLogModuleInfo *gWidgetDragLog;
extern PRLogModuleInfo *gWidgetDrawLog;

#define LOG(args) PR_LOG(gWidgetLog, 4, args)
#define LOGFOCUS(args) PR_LOG(gWidgetFocusLog, 4, args)
#define LOGDRAG(args) PR_LOG(gWidgetDragLog, 4, args)
#define LOGDRAW(args) PR_LOG(gWidgetDrawLog, 4, args)

#else

#define LOG(args)
#define LOGFOCUS(args)
#define LOGDRAG(args)
#define LOGDRAW(args)

#endif 

class gfxASurface;
class gfxPattern;
class nsDragService;
class nsPluginNativeWindowGtk;
#if defined(MOZ_X11) && defined(MOZ_HAVE_SHAREDMEMORYSYSV)
#  define MOZ_HAVE_SHMIMAGE
class nsShmImage;
#endif

class nsWindow : public nsBaseWidget
{
public:
    nsWindow();

    static void ReleaseGlobals();

    NS_DECL_ISUPPORTS_INHERITED
    
    void CommonCreate(nsIWidget *aParent, bool aListenForResizes);
    
    virtual nsresult DispatchEvent(mozilla::WidgetGUIEvent* aEvent,
                                   nsEventStatus& aStatus) override;
    
    
    virtual void OnDestroy(void) override;

    
    bool AreBoundsSane(void);

    
    NS_IMETHOD         Create(nsIWidget        *aParent,
                              nsNativeWidget   aNativeParent,
                              const nsIntRect  &aRect,
                              nsWidgetInitData *aInitData) override;
    NS_IMETHOD         Destroy(void) override;
    virtual nsIWidget *GetParent() override;
    virtual float      GetDPI() override;
    virtual double     GetDefaultScaleInternal() override; 
    virtual nsresult   SetParent(nsIWidget* aNewParent) override;
    NS_IMETHOD         SetModal(bool aModal) override;
    virtual bool       IsVisible() const override;
    NS_IMETHOD         ConstrainPosition(bool aAllowSlop,
                                         int32_t *aX,
                                         int32_t *aY) override;
    virtual void       SetSizeConstraints(const SizeConstraints& aConstraints) override;
    NS_IMETHOD         Move(double aX,
                            double aY) override;
    NS_IMETHOD         Show             (bool aState) override;
    NS_IMETHOD         Resize           (double aWidth,
                                         double aHeight,
                                         bool   aRepaint) override;
    NS_IMETHOD         Resize           (double aX,
                                         double aY,
                                         double aWidth,
                                         double aHeight,
                                         bool   aRepaint) override;
    virtual bool       IsEnabled() const override;


    NS_IMETHOD         PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                                   nsIWidget                  *aWidget,
                                   bool                        aActivate) override;
    void               SetZIndex(int32_t aZIndex) override;
    NS_IMETHOD         SetSizeMode(int32_t aMode) override;
    NS_IMETHOD         Enable(bool aState) override;
    NS_IMETHOD         SetFocus(bool aRaise = false) override;
    NS_IMETHOD         GetScreenBounds(nsIntRect &aRect) override;
    NS_IMETHOD         GetClientBounds(nsIntRect &aRect) override;
    virtual mozilla::gfx::IntSize GetClientSize() override;
    virtual nsIntPoint GetClientOffset() override;
    NS_IMETHOD         SetCursor(nsCursor aCursor) override;
    NS_IMETHOD         SetCursor(imgIContainer* aCursor,
                                 uint32_t aHotspotX, uint32_t aHotspotY) override;
    NS_IMETHOD         Invalidate(const nsIntRect &aRect) override;
    virtual void       Update() override;
    virtual void*      GetNativeData(uint32_t aDataType) override;
    void               SetNativeData(uint32_t aDataType, uintptr_t aVal) override;
    NS_IMETHOD         SetTitle(const nsAString& aTitle) override;
    NS_IMETHOD         SetIcon(const nsAString& aIconSpec) override;
    NS_IMETHOD         SetWindowClass(const nsAString& xulWinType) override;
    virtual mozilla::LayoutDeviceIntPoint WidgetToScreenOffset() override;
    NS_IMETHOD         EnableDragDrop(bool aEnable) override;
    NS_IMETHOD         CaptureMouse(bool aCapture) override;
    NS_IMETHOD         CaptureRollupEvents(nsIRollupListener *aListener,
                                           bool aDoCapture) override;
    NS_IMETHOD         GetAttention(int32_t aCycleCount) override;
    virtual nsresult   SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                           bool aIntersectWithExisting) override;
    virtual bool       HasPendingInputEvent() override;

    NS_IMETHOD         MakeFullScreen(bool aFullScreen,
                                      nsIScreen* aTargetScreen = nullptr) override;
    NS_IMETHOD         HideWindowChrome(bool aShouldHide) override;

    



    static guint32     GetLastUserInputTime();

    
    
    gint               ConvertBorderStyles(nsBorderStyle aStyle);

    
#if (MOZ_WIDGET_GTK == 2)
    gboolean           OnExposeEvent(GdkEventExpose *aEvent);
#else
    gboolean           OnExposeEvent(cairo_t *cr);
#endif
    gboolean           OnConfigureEvent(GtkWidget *aWidget,
                                        GdkEventConfigure *aEvent);
    void               OnContainerUnrealize();
    void               OnSizeAllocate(GtkAllocation *aAllocation);
    void               OnDeleteEvent();
    void               OnEnterNotifyEvent(GdkEventCrossing *aEvent);
    void               OnLeaveNotifyEvent(GdkEventCrossing *aEvent);
    void               OnMotionNotifyEvent(GdkEventMotion *aEvent);
    void               OnButtonPressEvent(GdkEventButton *aEvent);
    void               OnButtonReleaseEvent(GdkEventButton *aEvent);
    void               OnContainerFocusInEvent(GdkEventFocus *aEvent);
    void               OnContainerFocusOutEvent(GdkEventFocus *aEvent);
    gboolean           OnKeyPressEvent(GdkEventKey *aEvent);
    gboolean           OnKeyReleaseEvent(GdkEventKey *aEvent);
    void               OnScrollEvent(GdkEventScroll *aEvent);
    void               OnVisibilityNotifyEvent(GdkEventVisibility *aEvent);
    void               OnWindowStateEvent(GtkWidget *aWidget,
                                          GdkEventWindowState *aEvent);
    void               OnDragDataReceivedEvent(GtkWidget       *aWidget,
                                               GdkDragContext  *aDragContext,
                                               gint             aX,
                                               gint             aY,
                                               GtkSelectionData*aSelectionData,
                                               guint            aInfo,
                                               guint            aTime,
                                               gpointer         aData);

  mozilla::TemporaryRef<mozilla::gfx::DrawTarget> StartRemoteDrawing() override;

private:
    void               UpdateAlpha(gfxPattern* aPattern, nsIntRect aBoundsRect);

    void               NativeResize(int32_t aWidth,
                                    int32_t aHeight,
                                    bool    aRepaint);

    void               NativeResize(int32_t aX,
                                    int32_t aY,
                                    int32_t aWidth,
                                    int32_t aHeight,
                                    bool    aRepaint);

    void               NativeShow  (bool    aAction);
    void               SetHasMappedToplevel(bool aState);
    mozilla::LayoutDeviceIntSize GetSafeWindowSize(mozilla::LayoutDeviceIntSize aSize);

    void               EnsureGrabs  (void);
    void               GrabPointer  (guint32 aTime);
    void               ReleaseGrabs (void);

public:
    enum PluginType {
        PluginType_NONE = 0,   
        PluginType_XEMBED,     
        PluginType_NONXEMBED   
    };

    void               SetPluginType(PluginType aPluginType);
#ifdef MOZ_X11
    void               SetNonXEmbedPluginFocus(void);
    void               LoseNonXEmbedPluginFocus(void);
#endif 

    void               ThemeChanged(void);

#ifdef MOZ_X11
    Window             mOldFocusWindow;
#endif 

    static guint32     sLastButtonPressTime;

    NS_IMETHOD         BeginResizeDrag(mozilla::WidgetGUIEvent* aEvent,
                                       int32_t aHorizontal,
                                       int32_t aVertical) override;
    NS_IMETHOD         BeginMoveDrag(mozilla::WidgetMouseEvent* aEvent) override;

    MozContainer*      GetMozContainer() { return mContainer; }
    
    
    GtkWidget*         GetMozContainerWidget();
    GdkWindow*         GetGdkWindow() { return mGdkWindow; }
    bool               IsDestroyed() { return mIsDestroyed; }

    void               DispatchDragEvent(uint32_t aMsg,
                                         const nsIntPoint& aRefPoint,
                                         guint aTime);
    static void        UpdateDragStatus (GdkDragContext *aDragContext,
                                         nsIDragService *aDragService);
    
    
    bool               DispatchKeyDownEvent(GdkEventKey *aEvent,
                                            bool *aIsCancelled);

    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction) override;
    NS_IMETHOD_(InputContext) GetInputContext() override;
    virtual nsIMEUpdatePreference GetIMEUpdatePreference() override;
    bool ExecuteNativeKeyBindingRemapped(
                        NativeKeyBindingsType aType,
                        const mozilla::WidgetKeyboardEvent& aEvent,
                        DoCommandCallback aCallback,
                        void* aCallbackData,
                        uint32_t aGeckoKeyCode,
                        uint32_t aNativeKeyCode);
    NS_IMETHOD_(bool) ExecuteNativeKeyBinding(
                        NativeKeyBindingsType aType,
                        const mozilla::WidgetKeyboardEvent& aEvent,
                        DoCommandCallback aCallback,
                        void* aCallbackData) override;
    NS_IMETHOD GetToggledKeyState(uint32_t aKeyCode,
                                  bool* aLEDState) override;

    
    void               ResizeTransparencyBitmap();
    void               ApplyTransparencyBitmap();
    void               ClearTransparencyBitmap();

   virtual void        SetTransparencyMode(nsTransparencyMode aMode) override;
   virtual nsTransparencyMode GetTransparencyMode() override;
   virtual nsresult    ConfigureChildren(const nsTArray<Configuration>& aConfigurations) override;
   nsresult            UpdateTranslucentWindowAlphaInternal(const nsIntRect& aRect,
                                                            uint8_t* aAlphas, int32_t aStride);
    virtual gfxASurface *GetThebesSurface();

#if (MOZ_WIDGET_GTK == 2)
    static already_AddRefed<gfxASurface> GetSurfaceForGdkDrawable(GdkDrawable* aDrawable,
                                                                  const nsIntSize& aSize);
#endif
    NS_IMETHOD         ReparentNativeWidget(nsIWidget* aNewParent) override;

    virtual nsresult SynthesizeNativeMouseEvent(mozilla::LayoutDeviceIntPoint aPoint,
                                                uint32_t aNativeMessage,
                                                uint32_t aModifierFlags) override;

    virtual nsresult SynthesizeNativeMouseMove(mozilla::LayoutDeviceIntPoint aPoint) override
    { return SynthesizeNativeMouseEvent(aPoint, GDK_MOTION_NOTIFY, 0); }

protected:
    virtual ~nsWindow();

    
    void DispatchActivateEvent(void);
    void DispatchDeactivateEvent(void);
    void DispatchResized(int32_t aWidth, int32_t aHeight);

    
    void ReparentNativeWidgetInternal(nsIWidget* aNewParent,
                                      GtkWidget* aNewContainer,
                                      GdkWindow* aNewParentWindow,
                                      GtkWidget* aOldContainer);

    virtual nsresult NotifyIMEInternal(
                         const IMENotification& aIMENotification) override;

    nsCOMPtr<nsIWidget> mParent;
    
    bool                mIsTopLevel;
    
    bool                mIsDestroyed;

    
    
    bool                mNeedsResize;
    
    
    bool                mNeedsMove;
    
    bool                mListenForResizes;
    
    bool                mIsShown;
    bool                mNeedsShow;
    
    bool                mEnabled;
    
    bool                mCreated;

private:
    void               DestroyChildWindows();
    GtkWidget         *GetToplevelWidget();
    nsWindow          *GetContainerWindow();
    void               SetUrgencyHint(GtkWidget *top_window, bool state);
    void              *SetupPluginPort(void);
    void               SetDefaultIcon(void);
    void               InitButtonEvent(mozilla::WidgetMouseEvent& aEvent,
                                       GdkEventButton* aGdkEvent);
    bool               DispatchCommandEvent(nsIAtom* aCommand);
    bool               DispatchContentCommandEvent(int32_t aMsg);
    bool               CheckForRollup(gdouble aMouseX, gdouble aMouseY,
                                      bool aIsWheel, bool aAlwaysRollup);
    bool               GetDragInfo(mozilla::WidgetMouseEvent* aMouseEvent,
                                   GdkWindow** aWindow, gint* aButton,
                                   gint* aRootX, gint* aRootY);
    void               ClearCachedResources();

    GtkWidget          *mShell;
    MozContainer       *mContainer;
    GdkWindow          *mGdkWindow;

    uint32_t            mHasMappedToplevel : 1,
                        mIsFullyObscured : 1,
                        mRetryPointerGrab : 1;
    nsSizeMode          mSizeState;
    PluginType          mPluginType;

    int32_t             mTransparencyBitmapWidth;
    int32_t             mTransparencyBitmapHeight;

#if GTK_CHECK_VERSION(3,4,0)
    
    guint32             mLastScrollEventTime;
#endif

#ifdef MOZ_HAVE_SHMIMAGE
    
    nsRefPtr<nsShmImage>  mShmImage;
#endif
    nsRefPtr<gfxASurface> mThebesSurface;

#ifdef ACCESSIBILITY
    nsRefPtr<mozilla::a11y::Accessible> mRootAccessible;

    


    void                CreateRootAccessible();

    




    void                DispatchEventToRootAccessible(uint32_t aEventType);

    



    void                DispatchActivateEventAccessible();

    



    void                DispatchDeactivateEventAccessible();

    



    void                DispatchMaximizeEventAccessible();

    



    void                DispatchMinimizeEventAccessible();

    



    void                DispatchRestoreEventAccessible();
#endif

    
    void ResizePluginSocketWidget();

    
    nsPluginNativeWindowGtk* mPluginNativeWindow;

    
    static GdkCursor   *gsGtkCursorCache[eCursorCount];

    
    bool         mIsTransparent;
    
    
    
    gchar*       mTransparencyBitmap;
 
    
    void   InitDragEvent(mozilla::WidgetDragEvent& aEvent);

    float              mLastMotionPressure;

    
    
    nsSizeMode         mLastSizeMode;

    static bool DragInProgress(void);

    void DispatchMissedButtonReleases(GdkEventCrossing *aGdkEvent);

    
    virtual LayerManager* GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr) override;
#if (MOZ_WIDGET_GTK == 3)
    gfxASurface* GetThebesSurface(cairo_t *cr);
#endif

    void CleanLayerManagerRecursive();

    












    nsRefPtr<nsGtkIMModule> mIMModule;

    
    gint GdkScaleFactor();

    
    gint DevicePixelsToGdkCoordRoundUp(int pixels);
    gint DevicePixelsToGdkCoordRoundDown(int pixels);
    GdkPoint DevicePixelsToGdkPointRoundDown(nsIntPoint point);
    GdkRectangle DevicePixelsToGdkRectRoundOut(nsIntRect rect);

    
    int GdkCoordToDevicePixels(gint coord);
    mozilla::LayoutDeviceIntPoint GdkPointToDevicePixels(GdkPoint point);
    nsIntRect GdkRectToDevicePixels(GdkRectangle rect);
};

class nsChildWindow : public nsWindow {
public:
    nsChildWindow();
    ~nsChildWindow();
};

#endif
