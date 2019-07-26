






#ifndef __nsWindow_h__
#define __nsWindow_h__

#include "mozilla/ipc/SharedMemorySysV.h"

#include "nsAutoPtr.h"

#include "mozcontainer.h"
#include "nsWeakReference.h"

#include "nsIDragService.h"
#include "nsITimer.h"
#include "nsGkAtoms.h"

#include "gfxASurface.h"

#include "nsBaseWidget.h"
#include "nsGUIEvent.h"
#include <gdk/gdk.h>
#include <gtk/gtk.h>

#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 

#ifdef ACCESSIBILITY
#include "mozilla/a11y/Accessible.h"
#endif

#include "nsGtkIMModule.h"

#ifdef MOZ_LOGGING


#define FORCE_PR_LOG

#include "prlog.h"
#include "nsTArray.h"

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

class nsDragService;
#if defined(MOZ_X11) && defined(MOZ_HAVE_SHAREDMEMORYSYSV)
#  define MOZ_HAVE_SHMIMAGE

class nsShmImage;
#endif

class nsWindow : public nsBaseWidget, public nsSupportsWeakReference
{
public:
    nsWindow();
    virtual ~nsWindow();

    static void ReleaseGlobals();

    NS_DECL_ISUPPORTS_INHERITED
    
    void CommonCreate(nsIWidget *aParent, bool aListenForResizes);
    
    virtual nsresult DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus);
    
    
    void OnDestroy(void);

    
    bool AreBoundsSane(void);

    
    NS_IMETHOD         Create(nsIWidget        *aParent,
                              nsNativeWidget   aNativeParent,
                              const nsIntRect  &aRect,
                              nsDeviceContext *aContext,
                              nsWidgetInitData *aInitData);
    NS_IMETHOD         Destroy(void);
    virtual nsIWidget *GetParent();
    virtual float      GetDPI();
    virtual nsresult   SetParent(nsIWidget* aNewParent);
    NS_IMETHOD         SetModal(bool aModal);
    virtual bool       IsVisible() const;
    NS_IMETHOD         ConstrainPosition(bool aAllowSlop,
                                         int32_t *aX,
                                         int32_t *aY);
    virtual void       SetSizeConstraints(const SizeConstraints& aConstraints);
    NS_IMETHOD         Move(double aX,
                            double aY);
    NS_IMETHOD         Show             (bool aState);
    NS_IMETHOD         Resize           (double aWidth,
                                         double aHeight,
                                         bool   aRepaint);
    NS_IMETHOD         Resize           (double aX,
                                         double aY,
                                         double aWidth,
                                         double aHeight,
                                         bool   aRepaint);
    virtual bool       IsEnabled() const;


    NS_IMETHOD         PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                                   nsIWidget                  *aWidget,
                                   bool                        aActivate);
    NS_IMETHOD         SetZIndex(int32_t aZIndex);
    NS_IMETHOD         SetSizeMode(int32_t aMode);
    NS_IMETHOD         Enable(bool aState);
    NS_IMETHOD         SetFocus(bool aRaise = false);
    NS_IMETHOD         GetScreenBounds(nsIntRect &aRect);
    NS_IMETHOD         GetClientBounds(nsIntRect &aRect);
    virtual nsIntPoint GetClientOffset();
    NS_IMETHOD         SetForegroundColor(const nscolor &aColor);
    NS_IMETHOD         SetBackgroundColor(const nscolor &aColor);
    NS_IMETHOD         SetCursor(nsCursor aCursor);
    NS_IMETHOD         SetCursor(imgIContainer* aCursor,
                                 uint32_t aHotspotX, uint32_t aHotspotY);
    NS_IMETHOD         Invalidate(const nsIntRect &aRect);
    virtual void*      GetNativeData(uint32_t aDataType);
    NS_IMETHOD         SetTitle(const nsAString& aTitle);
    NS_IMETHOD         SetIcon(const nsAString& aIconSpec);
    NS_IMETHOD         SetWindowClass(const nsAString& xulWinType);
    virtual nsIntPoint WidgetToScreenOffset();
    NS_IMETHOD         EnableDragDrop(bool aEnable);
    NS_IMETHOD         CaptureMouse(bool aCapture);
    NS_IMETHOD         CaptureRollupEvents(nsIRollupListener *aListener,
                                           bool aDoCapture);
    NS_IMETHOD         GetAttention(int32_t aCycleCount);

    virtual bool       HasPendingInputEvent();

    NS_IMETHOD         MakeFullScreen(bool aFullScreen);
    NS_IMETHOD         HideWindowChrome(bool aShouldHide);

    



    static guint32     GetLastUserInputTime();

    
    
    gint               ConvertBorderStyles(nsBorderStyle aStyle);

    
#if defined(MOZ_WIDGET_GTK2)
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

private:
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
    nsIntSize          GetSafeWindowSize(nsIntSize aSize);

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

    NS_IMETHOD         BeginResizeDrag(nsGUIEvent* aEvent, int32_t aHorizontal, int32_t aVertical);
    NS_IMETHOD         BeginMoveDrag(nsMouseEvent* aEvent);

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

    NS_IMETHOD NotifyIME(NotificationToIME aNotification) MOZ_OVERRIDE;
    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction);
    NS_IMETHOD_(InputContext) GetInputContext();
    NS_IMETHOD GetToggledKeyState(uint32_t aKeyCode, bool* aLEDState);

    
    void               ResizeTransparencyBitmap();
    void               ApplyTransparencyBitmap();
    void               ClearTransparencyBitmap();

   virtual void        SetTransparencyMode(nsTransparencyMode aMode);
   virtual nsTransparencyMode GetTransparencyMode();
   virtual nsresult    ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
   nsresult            UpdateTranslucentWindowAlphaInternal(const nsIntRect& aRect,
                                                            uint8_t* aAlphas, int32_t aStride);

#if defined(MOZ_WIDGET_GTK2)
    gfxASurface       *GetThebesSurface();

    static already_AddRefed<gfxASurface> GetSurfaceForGdkDrawable(GdkDrawable* aDrawable,
                                                                  const nsIntSize& aSize);
#else
    gfxASurface       *GetThebesSurface(cairo_t *cr);
#endif
    NS_IMETHOD         ReparentNativeWidget(nsIWidget* aNewParent);

    virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                uint32_t aNativeMessage,
                                                uint32_t aModifierFlags);

    virtual nsresult SynthesizeNativeMouseMove(nsIntPoint aPoint)
    { return SynthesizeNativeMouseEvent(aPoint, GDK_MOTION_NOTIFY, 0); }

protected:
    
    void DispatchActivateEvent(void);
    void DispatchDeactivateEvent(void);
    void DispatchResized(int32_t aWidth, int32_t aHeight);

    
    void ReparentNativeWidgetInternal(nsIWidget* aNewParent,
                                      GtkWidget* aNewContainer,
                                      GdkWindow* aNewParentWindow,
                                      GtkWidget* aOldContainer);
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
    void               InitButtonEvent(nsMouseEvent &aEvent, GdkEventButton *aGdkEvent);
    bool               DispatchCommandEvent(nsIAtom* aCommand);
    bool               DispatchContentCommandEvent(int32_t aMsg);
    void               SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                           bool aIntersectWithExisting);
    bool               CheckForRollup(gdouble aMouseX, gdouble aMouseY,
                                      bool aIsWheel, bool aAlwaysRollup);
    bool               GetDragInfo(nsMouseEvent* aMouseEvent,
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

    
    static GdkCursor   *gsGtkCursorCache[eCursorCount];

    
    bool         mIsTransparent;
    
    
    
    gchar*       mTransparencyBitmap;
 
    
    void   InitDragEvent         (nsDragEvent &aEvent);

    float              mLastMotionPressure;

    
    
    nsSizeMode         mLastSizeMode;

    static bool DragInProgress(void);

    void DispatchMissedButtonReleases(GdkEventCrossing *aGdkEvent);

    
    virtual LayerManager* GetLayerManager(PLayersChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr) MOZ_OVERRIDE;

    void CleanLayerManagerRecursive();

    












    nsRefPtr<nsGtkIMModule> mIMModule;
};

class nsChildWindow : public nsWindow {
public:
    nsChildWindow();
    ~nsChildWindow();
};

#endif
