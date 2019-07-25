






































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

#ifdef MOZ_DFB
#include <gdk/gdkdirectfb.h>
#endif 

#ifdef MOZ_X11
#include <gdk/gdkx.h>
#endif 

#ifdef ACCESSIBILITY
#include "nsAccessible.h"
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
    
    
    void InitKeyEvent(nsKeyEvent &aEvent, GdkEventKey *aGdkEvent);

    void DispatchActivateEvent(void);
    void DispatchDeactivateEvent(void);
    void DispatchResizeEvent(nsIntRect &aRect, nsEventStatus &aStatus);

    virtual nsresult DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus);
    
    
    void OnDestroy(void);

    
    bool AreBoundsSane(void);

    
    NS_IMETHOD         Create(nsIWidget        *aParent,
                              nsNativeWidget   aNativeParent,
                              const nsIntRect  &aRect,
                              EVENT_CALLBACK   aHandleEventFunction,
                              nsDeviceContext *aContext,
                              nsWidgetInitData *aInitData);
    NS_IMETHOD         Destroy(void);
    virtual nsIWidget *GetParent();
    virtual float      GetDPI();
    virtual nsresult   SetParent(nsIWidget* aNewParent);
    NS_IMETHOD         SetModal(bool aModal);
    NS_IMETHOD         IsVisible(bool & aState);
    NS_IMETHOD         ConstrainPosition(bool aAllowSlop,
                                         PRInt32 *aX,
                                         PRInt32 *aY);
    NS_IMETHOD         Move(PRInt32 aX,
                            PRInt32 aY);
    NS_IMETHOD         Show             (bool aState);
    NS_IMETHOD         Resize           (PRInt32 aWidth,
                                         PRInt32 aHeight,
                                         bool    aRepaint);
    NS_IMETHOD         Resize           (PRInt32 aX,
                                         PRInt32 aY,
                                         PRInt32 aWidth,
                                         PRInt32 aHeight,
                                         bool     aRepaint);
    NS_IMETHOD         IsEnabled        (bool *aState);


    NS_IMETHOD         PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                                   nsIWidget                  *aWidget,
                                   bool                        aActivate);
    NS_IMETHOD         SetZIndex(PRInt32 aZIndex);
    NS_IMETHOD         SetSizeMode(PRInt32 aMode);
    NS_IMETHOD         Enable(bool aState);
    NS_IMETHOD         SetFocus(bool aRaise = false);
    NS_IMETHOD         GetScreenBounds(nsIntRect &aRect);
    virtual nsIntPoint GetClientOffset();
    NS_IMETHOD         SetForegroundColor(const nscolor &aColor);
    NS_IMETHOD         SetBackgroundColor(const nscolor &aColor);
    NS_IMETHOD         SetCursor(nsCursor aCursor);
    NS_IMETHOD         SetCursor(imgIContainer* aCursor,
                                 PRUint32 aHotspotX, PRUint32 aHotspotY);
    NS_IMETHOD         Invalidate(const nsIntRect &aRect,
                                  bool             aIsSynchronous);
    NS_IMETHOD         Update();
    virtual void*      GetNativeData(PRUint32 aDataType);
    NS_IMETHOD         SetTitle(const nsAString& aTitle);
    NS_IMETHOD         SetIcon(const nsAString& aIconSpec);
    NS_IMETHOD         SetWindowClass(const nsAString& xulWinType);
    virtual nsIntPoint WidgetToScreenOffset();
    NS_IMETHOD         EnableDragDrop(bool aEnable);
    NS_IMETHOD         CaptureMouse(bool aCapture);
    NS_IMETHOD         CaptureRollupEvents(nsIRollupListener *aListener,
                                           bool aDoCapture,
                                           bool aConsumeRollupEvent);
    NS_IMETHOD         GetAttention(PRInt32 aCycleCount);

    virtual bool       HasPendingInputEvent();

    NS_IMETHOD         MakeFullScreen(bool aFullScreen);
    NS_IMETHOD         HideWindowChrome(bool aShouldHide);

    
    
    gint               ConvertBorderStyles(nsBorderStyle aStyle);

    
#if defined(MOZ_WIDGET_GTK2)
    gboolean           OnExposeEvent(GdkEventExpose *aEvent);
#else
    gboolean           OnExposeEvent(cairo_t *cr);
#endif
    gboolean           OnConfigureEvent(GtkWidget *aWidget,
                                        GdkEventConfigure *aEvent);
    void               OnContainerUnrealize(GtkWidget *aWidget);
    void               OnSizeAllocate(GtkWidget *aWidget,
                                      GtkAllocation *aAllocation);
    void               OnDeleteEvent(GtkWidget *aWidget,
                                     GdkEventAny *aEvent);
    void               OnEnterNotifyEvent(GtkWidget *aWidget,
                                          GdkEventCrossing *aEvent);
    void               OnLeaveNotifyEvent(GtkWidget *aWidget,
                                          GdkEventCrossing *aEvent);
    void               OnMotionNotifyEvent(GtkWidget *aWidget,
                                           GdkEventMotion *aEvent);
    void               OnButtonPressEvent(GtkWidget *aWidget,
                                          GdkEventButton *aEvent);
    void               OnButtonReleaseEvent(GtkWidget *aWidget,
                                            GdkEventButton *aEvent);
    void               OnContainerFocusInEvent(GtkWidget *aWidget,
                                               GdkEventFocus *aEvent);
    void               OnContainerFocusOutEvent(GtkWidget *aWidget,
                                                GdkEventFocus *aEvent);
    gboolean           OnKeyPressEvent(GtkWidget *aWidget,
                                       GdkEventKey *aEvent);
    gboolean           OnKeyReleaseEvent(GtkWidget *aWidget,
                                         GdkEventKey *aEvent);
    void               OnScrollEvent(GtkWidget *aWidget,
                                     GdkEventScroll *aEvent);
    void               OnVisibilityNotifyEvent(GtkWidget *aWidget,
                                               GdkEventVisibility *aEvent);
    void               OnWindowStateEvent(GtkWidget *aWidget,
                                          GdkEventWindowState *aEvent);
    gboolean           OnDragMotionEvent(GtkWidget       *aWidget,
                                         GdkDragContext  *aDragContext,
                                         gint             aX,
                                         gint             aY,
                                         guint            aTime,
                                         gpointer         aData);
    void               OnDragLeaveEvent(GtkWidget *      aWidget,
                                        GdkDragContext   *aDragContext,
                                        guint            aTime,
                                        gpointer         aData);
    gboolean           OnDragDropEvent(GtkWidget        *aWidget,
                                       GdkDragContext   *aDragContext,
                                       gint             aX,
                                       gint             aY,
                                       guint            aTime,
                                       gpointer         aData);
    void               OnDragDataReceivedEvent(GtkWidget       *aWidget,
                                               GdkDragContext  *aDragContext,
                                               gint             aX,
                                               gint             aY,
                                               GtkSelectionData*aSelectionData,
                                               guint            aInfo,
                                               guint            aTime,
                                               gpointer         aData);
    void               OnDragLeave(void);
    void               OnDragEnter(nscoord aX, nscoord aY);

    virtual void       NativeResize(PRInt32 aWidth,
                                    PRInt32 aHeight,
                                    bool    aRepaint);

    virtual void       NativeResize(PRInt32 aX,
                                    PRInt32 aY,
                                    PRInt32 aWidth,
                                    PRInt32 aHeight,
                                    bool    aRepaint);

    virtual void       NativeShow  (bool    aAction);
    void               SetHasMappedToplevel(bool aState);
    nsIntSize          GetSafeWindowSize(nsIntSize aSize);

    void               EnsureGrabs  (void);
    void               GrabPointer  (void);
    void               ReleaseGrabs (void);

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

    void CheckNeedDragLeaveEnter(nsWindow* aInnerMostWidget,
                                 nsIDragService* aDragService,
                                 GdkDragContext *aDragContext,
                                 nscoord aX, nscoord aY);

#ifdef MOZ_X11
    Window             mOldFocusWindow;
#endif 

    static guint32     sLastButtonPressTime;
    static guint32     sLastButtonReleaseTime;

    NS_IMETHOD         BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);
    NS_IMETHOD         BeginMoveDrag(nsMouseEvent* aEvent);

    MozContainer*      GetMozContainer() { return mContainer; }
    GdkWindow*         GetGdkWindow() { return mGdkWindow; }
    bool               IsDestroyed() { return mIsDestroyed; }

    
    
    bool               DispatchKeyDownEvent(GdkEventKey *aEvent,
                                            bool *aIsCancelled);

    NS_IMETHOD ResetInputState();
    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction);
    NS_IMETHOD_(InputContext) GetInputContext();
    NS_IMETHOD CancelIMEComposition();
    NS_IMETHOD OnIMEFocusChange(bool aFocus);
    NS_IMETHOD GetToggledKeyState(PRUint32 aKeyCode, bool* aLEDState);

   void                ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight);
   void                ApplyTransparencyBitmap();
   virtual void        SetTransparencyMode(nsTransparencyMode aMode);
   virtual nsTransparencyMode GetTransparencyMode();
   virtual nsresult    ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
   nsresult            UpdateTranslucentWindowAlphaInternal(const nsIntRect& aRect,
                                                            PRUint8* aAlphas, PRInt32 aStride);

#if defined(MOZ_WIDGET_GTK2)
    gfxASurface       *GetThebesSurface();

    static already_AddRefed<gfxASurface> GetSurfaceForGdkDrawable(GdkDrawable* aDrawable,
                                                                  const nsIntSize& aSize);
#else
    gfxASurface       *GetThebesSurface(cairo_t *cr);
#endif
    NS_IMETHOD         ReparentNativeWidget(nsIWidget* aNewParent);

#ifdef ACCESSIBILITY
    static bool        sAccessibilityEnabled;
#endif
protected:
    
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
    void               GetToplevelWidget(GtkWidget **aWidget);
    GtkWidget         *GetMozContainerWidget();
    nsWindow          *GetContainerWindow();
    void               SetUrgencyHint(GtkWidget *top_window, bool state);
    void              *SetupPluginPort(void);
    nsresult           SetWindowIconList(const nsTArray<nsCString> &aIconList);
    void               SetDefaultIcon(void);
    void               InitButtonEvent(nsMouseEvent &aEvent, GdkEventButton *aGdkEvent);
    bool               DispatchCommandEvent(nsIAtom* aCommand);
    bool               DispatchContentCommandEvent(PRInt32 aMsg);
    void               SetWindowClipRegion(const nsTArray<nsIntRect>& aRects,
                                           bool aIntersectWithExisting);
    bool               GetDragInfo(nsMouseEvent* aMouseEvent,
                                   GdkWindow** aWindow, gint* aButton,
                                   gint* aRootX, gint* aRootY);
    void               ClearCachedResources();

    GtkWidget          *mShell;
    MozContainer       *mContainer;
    GdkWindow          *mGdkWindow;

    GtkWindowGroup     *mWindowGroup;

    PRUint32            mHasMappedToplevel : 1,
                        mIsFullyObscured : 1,
                        mRetryPointerGrab : 1;
    GtkWindow          *mTransientParent;
    PRInt32             mSizeState;
    PluginType          mPluginType;

    PRInt32             mTransparencyBitmapWidth;
    PRInt32             mTransparencyBitmapHeight;

#ifdef MOZ_HAVE_SHMIMAGE
    
    nsRefPtr<nsShmImage>  mShmImage;
#endif
    nsRefPtr<gfxASurface> mThebesSurface;

#ifdef MOZ_DFB
    int                    mDFBCursorX;
    int                    mDFBCursorY;
    PRUint32               mDFBCursorCount;
    IDirectFB             *mDFB;
    IDirectFBDisplayLayer *mDFBLayer;
#endif

#ifdef ACCESSIBILITY
    nsRefPtr<nsAccessible> mRootAccessible;

    


    void                CreateRootAccessible();

    



    nsAccessible       *DispatchAccessibleEvent();

    




    void                DispatchEventToRootAccessible(PRUint32 aEventType);

    



    void                DispatchActivateEventAccessible();

    



    void                DispatchDeactivateEventAccessible();

    



    void                DispatchMaximizeEventAccessible();

    



    void                DispatchMinimizeEventAccessible();

    



    void                DispatchRestoreEventAccessible();
#endif

    
    static GdkCursor   *gsGtkCursorCache[eCursorCount];

    
    bool         mIsTransparent;
    
    
    
    gchar*       mTransparencyBitmap;
 
    
    
    static nsWindow    *sLastDragMotionWindow;
    void   InitDragEvent         (nsDragEvent &aEvent);
    void   UpdateDragStatus      (GdkDragContext *aDragContext,
                                  nsIDragService *aDragService);

    nsCOMPtr<nsITimer> mDragLeaveTimer;
    float              mLastMotionPressure;

    
    
    nsSizeMode         mLastSizeMode;

    static bool        sIsDraggingOutOf;
    
    static bool DragInProgress(void);

    void         FireDragLeaveTimer       (void);
    static void  DragLeaveTimerCallback  (nsITimer *aTimer, void *aClosure);

    void DispatchMissedButtonReleases(GdkEventCrossing *aGdkEvent);

    












    nsRefPtr<nsGtkIMModule> mIMModule;
};

class nsChildWindow : public nsWindow {
public:
    nsChildWindow();
    ~nsChildWindow();
};

#endif 
