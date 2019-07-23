






































#ifndef __nsWindow_h__
#define __nsWindow_h__

#include "nsAutoPtr.h"

#include "nsCommonWidget.h"

#include "mozcontainer.h"
#include "mozdrawingarea.h"
#include "nsWeakReference.h"

#include "nsIDragService.h"
#include "nsITimer.h"

#include <gtk/gtk.h>

#include <gdk/gdkx.h>
#include <gtk/gtkwindow.h>

#ifdef ACCESSIBILITY
#include "nsIAccessNode.h"
#include "nsIAccessible.h"
#endif

#ifdef USE_XIM
#include <gtk/gtkimmulticontext.h>
#include "pldhash.h"
#include "nsIKBStateControl.h"
#endif

class nsWindow : public nsCommonWidget, public nsSupportsWeakReference
#ifdef USE_XIM
                ,public nsIKBStateControl
#endif
{
public:
    nsWindow();
    virtual ~nsWindow();

    static void ReleaseGlobals();

    NS_DECL_ISUPPORTS_INHERITED

    
    NS_IMETHOD         Create(nsIWidget        *aParent,
                              const nsRect     &aRect,
                              EVENT_CALLBACK   aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell      *aAppShell,
                              nsIToolkit       *aToolkit,
                              nsWidgetInitData *aInitData);
    NS_IMETHOD         Create(nsNativeWidget aParent,
                              const nsRect     &aRect,
                              EVENT_CALLBACK   aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell      *aAppShell,
                              nsIToolkit       *aToolkit,
                              nsWidgetInitData *aInitData);
    NS_IMETHOD         Destroy(void);
    NS_IMETHOD         SetParent(nsIWidget* aNewParent);
    NS_IMETHOD         SetModal(PRBool aModal);
    NS_IMETHOD         IsVisible(PRBool & aState);
    NS_IMETHOD         ConstrainPosition(PRBool aAllowSlop,
                                         PRInt32 *aX,
                                         PRInt32 *aY);
    NS_IMETHOD         Move(PRInt32 aX,
                            PRInt32 aY);
    NS_IMETHOD         PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                                   nsIWidget                  *aWidget,
                                   PRBool                      aActivate);
    NS_IMETHOD         SetZIndex(PRInt32 aZIndex);
    NS_IMETHOD         SetSizeMode(PRInt32 aMode);
    NS_IMETHOD         Enable(PRBool aState);
    NS_IMETHOD         SetFocus(PRBool aRaise = PR_FALSE);
    NS_IMETHOD         GetScreenBounds(nsRect &aRect);
    NS_IMETHOD         SetForegroundColor(const nscolor &aColor);
    NS_IMETHOD         SetBackgroundColor(const nscolor &aColor);
    NS_IMETHOD         SetCursor(nsCursor aCursor);
    NS_IMETHOD         SetCursor(imgIContainer* aCursor,
                                 PRUint32 aHotspotX, PRUint32 aHotspotY);
    NS_IMETHOD         Validate();
    NS_IMETHOD         Invalidate(PRBool aIsSynchronous);
    NS_IMETHOD         Invalidate(const nsRect &aRect,
                                  PRBool        aIsSynchronous);
    NS_IMETHOD         InvalidateRegion(const nsIRegion *aRegion,
                                        PRBool           aIsSynchronous);
    NS_IMETHOD         Update();
    NS_IMETHOD         SetColorMap(nsColorMap *aColorMap);
    NS_IMETHOD         Scroll(PRInt32  aDx,
                              PRInt32  aDy,
                              nsRect  *aClipRect);
    NS_IMETHOD         ScrollWidgets(PRInt32 aDx,
                                     PRInt32 aDy);
    NS_IMETHOD         ScrollRect(nsRect  &aSrcRect,
                                  PRInt32  aDx,
                                  PRInt32  aDy);
    virtual void*      GetNativeData(PRUint32 aDataType);
    NS_IMETHOD         SetBorderStyle(nsBorderStyle aBorderStyle);
    NS_IMETHOD         SetTitle(const nsAString& aTitle);
    NS_IMETHOD         SetIcon(const nsAString& aIconSpec);
    NS_IMETHOD         SetWindowClass(const nsAString& xulWinType);
    NS_IMETHOD         SetMenuBar(nsIMenuBar * aMenuBar);
    NS_IMETHOD         ShowMenuBar(PRBool aShow);
    NS_IMETHOD         WidgetToScreen(const nsRect& aOldRect,
                                      nsRect& aNewRect);
    NS_IMETHOD         ScreenToWidget(const nsRect& aOldRect,
                                      nsRect& aNewRect);
    NS_IMETHOD         BeginResizingChildren(void);
    NS_IMETHOD         EndResizingChildren(void);
    NS_IMETHOD         EnableDragDrop(PRBool aEnable);
    virtual void       ConvertToDeviceCoordinates(nscoord &aX,
                                                  nscoord &aY);
    NS_IMETHOD         PreCreateWidget(nsWidgetInitData *aWidgetInitData);
    NS_IMETHOD         CaptureMouse(PRBool aCapture);
    NS_IMETHOD         CaptureRollupEvents(nsIRollupListener *aListener,
                                           PRBool aDoCapture,
                                           PRBool aConsumeRollupEvent);
    NS_IMETHOD         GetAttention(PRInt32 aCycleCount);
    NS_IMETHOD         MakeFullScreen(PRBool aFullScreen);
    NS_IMETHOD         HideWindowChrome(PRBool aShouldHide);

    
    void               LoseFocus();
    gint               ConvertBorderStyles(nsBorderStyle aStyle);

    
    gboolean           OnExposeEvent(GtkWidget *aWidget,
                                     GdkEventExpose *aEvent);
    gboolean           OnConfigureEvent(GtkWidget *aWidget,
                                        GdkEventConfigure *aEvent);
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
                                         void            *aData);
    void               OnDragLeaveEvent(GtkWidget *      aWidget,
                                        GdkDragContext   *aDragContext,
                                        guint            aTime,
                                        gpointer         aData);
    gboolean           OnDragDropEvent(GtkWidget        *aWidget,
                                       GdkDragContext   *aDragContext,
                                       gint             aX,
                                       gint             aY,
                                       guint            aTime,
                                       gpointer         *aData);
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


    nsresult           NativeCreate(nsIWidget        *aParent,
                                    nsNativeWidget    aNativeParent,
                                    const nsRect     &aRect,
                                    EVENT_CALLBACK    aHandleEventFunction,
                                    nsIDeviceContext *aContext,
                                    nsIAppShell      *aAppShell,
                                    nsIToolkit       *aToolkit,
                                    nsWidgetInitData *aInitData);

    void               NativeResize(PRInt32 aWidth,
                                    PRInt32 aHeight,
                                    PRBool  aRepaint);

    void               NativeResize(PRInt32 aX,
                                    PRInt32 aY,
                                    PRInt32 aWidth,
                                    PRInt32 aHeight,
                                    PRBool  aRepaint);

    void               NativeShow  (PRBool  aAction);

    void               EnsureGrabs  (void);
    void               GrabPointer  (void);
    void               GrabKeyboard (void);
    void               ReleaseGrabs (void);

    enum PluginType {
        PluginType_NONE = 0,   
        PluginType_XEMBED,     
        PluginType_NONXEMBED   
    };

    void               SetPluginType(PluginType aPluginType);
    void               SetNonXEmbedPluginFocus(void);
    void               LoseNonXEmbedPluginFocus(void);

    void               ThemeChanged(void);

    Window             mOldFocusWindow;

    static guint32     mLastButtonPressTime;
    static guint32     mLastButtonReleaseTime;

#ifdef USE_XIM
    void               IMEInitData       (void);
    void               IMEReleaseData    (void);
    void               IMEDestroyContext (void);
    void               IMESetFocus       (void);
    void               IMELoseFocus      (void);
    void               IMEComposeStart   (void);
    void               IMEComposeText    (const PRUnichar *aText,
                                          const PRInt32 aLen,
                                          const gchar *aPreeditString,
                                          const gint aCursorPos,
                                          const PangoAttrList *aFeedback);
    void               IMEComposeEnd     (void);
    GtkIMContext*      IMEGetContext     (void);
    nsWindow*          IMEGetOwningWindow(void);
    PRBool             IMEIsEnabled      (void);
    nsWindow*          IMEComposingWindow(void);
    void               IMECreateContext  (void);
    PRBool             IMEFilterEvent    (GdkEventKey *aEvent);

    






    struct nsIMEData {
        
        GtkIMContext       *mContext;
        
        
        
        
        GtkIMContext       *mDummyContext;
        
        
        
        
        
        
        
        nsWindow           *mComposingWindow;
        
        
        nsWindow           *mOwner;
        
        
        PRUint32           mRefCount;
        
        PRUint32           mEnabled;
        nsIMEData(nsWindow* aOwner) {
            mContext         = nsnull;
            mDummyContext    = nsnull;
            mComposingWindow = nsnull;
            mOwner           = aOwner;
            mRefCount        = 1;
            mEnabled         = nsIKBStateControl::IME_STATUS_ENABLED;
        }
    };
    nsIMEData          *mIMEData;

    
    NS_IMETHOD ResetInputState();
    NS_IMETHOD SetIMEOpenState(PRBool aState);
    NS_IMETHOD GetIMEOpenState(PRBool* aState);
    NS_IMETHOD SetIMEEnabled(PRUint32 aState);
    NS_IMETHOD GetIMEEnabled(PRUint32* aState);
    NS_IMETHOD CancelIMEComposition();
    NS_IMETHOD GetToggledKeyState(PRUint32 aKeyCode, PRBool* aLEDState);

#endif

   void                ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight);
   void                ApplyTransparencyBitmap();
#ifdef MOZ_XUL
   NS_IMETHOD          SetWindowTranslucency(PRBool aTransparent);
   NS_IMETHOD          GetWindowTranslucency(PRBool& aTransparent);
   nsresult            UpdateTranslucentWindowAlphaInternal(const nsRect& aRect,
                                                            PRUint8* aAlphas, PRInt32 aStride);
   NS_IMETHOD          UpdateTranslucentWindowAlpha(const nsRect& aRect, PRUint8* aAlphas);
#endif

    gfxASurface       *GetThebesSurface();

#ifdef ACCESSIBILITY
    static PRBool      sAccessibilityEnabled;
#endif

private:
    void               GetToplevelWidget(GtkWidget **aWidget);
    void               GetContainerWindow(nsWindow  **aWindow);
    void               SetUrgencyHint(GtkWidget *top_window, PRBool state);
    void              *SetupPluginPort(void);
    nsresult           SetWindowIconList(const nsCStringArray &aIconList);
    void               SetDefaultIcon(void);

    GtkWidget          *mShell;
    MozContainer       *mContainer;
    MozDrawingarea     *mDrawingarea;

    GtkWindowGroup     *mWindowGroup;

    PRUint32            mContainerGotFocus : 1,
                        mContainerLostFocus : 1,
                        mContainerBlockFocus : 1,
                        mIsVisible : 1,
                        mRetryPointerGrab : 1,
                        mActivatePending : 1,
                        mRetryKeyboardGrab : 1;
    GtkWindow          *mTransientParent;
    PRInt32             mSizeState;
    PluginType          mPluginType;

    PRUint32            mTransparencyBitmapWidth;
    PRUint32            mTransparencyBitmapHeight;

    nsRefPtr<gfxASurface> mThebesSurface;

#ifdef ACCESSIBILITY
    nsCOMPtr<nsIAccessible> mRootAccessible;
    void                CreateRootAccessible();
    void                GetRootAccessible(nsIAccessible** aAccessible);
    void                DispatchActivateEvent(void);
    void                DispatchDeactivateEvent(void);
    NS_IMETHOD_(PRBool) DispatchAccessibleEvent(nsIAccessible** aAccessible);
#endif

    
    static GdkCursor   *gsGtkCursorCache[eCursorCount];

    
    PRBool       mIsTranslucent;
    
    
    
    gchar*       mTransparencyBitmap;
 
    
    
    static nsWindow    *mLastDragMotionWindow;
    void   InitDragEvent         (nsMouseEvent &aEvent);
    void   UpdateDragStatus      (nsMouseEvent &aEvent,
                                  GdkDragContext *aDragContext,
                                  nsIDragService *aDragService);

    
    
    GtkWidget         *mDragMotionWidget;
    GdkDragContext    *mDragMotionContext;
    gint               mDragMotionX;
    gint               mDragMotionY;
    guint              mDragMotionTime;
    guint              mDragMotionTimerID;
    nsCOMPtr<nsITimer> mDragLeaveTimer;

    static PRBool      sIsDraggingOutOf;
    
    static PRBool DragInProgress(void);

    void         ResetDragMotionTimer     (GtkWidget      *aWidget,
                                           GdkDragContext *aDragContext,
                                           gint           aX,
                                           gint           aY,
                                           guint          aTime);
    void         FireDragMotionTimer      (void);
    void         FireDragLeaveTimer       (void);
    static guint DragMotionTimerCallback (gpointer aClosure);
    static void  DragLeaveTimerCallback  (nsITimer *aTimer, void *aClosure);

    
    PRUint32 mKeyDownFlags[8];

    
    PRUint32* GetFlagWord32(PRUint32 aKeyCode, PRUint32* aMask) {
        
        NS_ASSERTION((aKeyCode <= 0xFF), "Invalid DOM Key Code");
        aKeyCode &= 0xFF;

        
        *aMask = PRUint32(1) << (aKeyCode & 0x1F);
        return &mKeyDownFlags[(aKeyCode >> 5)];
    }

    PRBool IsKeyDown(PRUint32 aKeyCode) {
        PRUint32 mask;
        PRUint32* flag = GetFlagWord32(aKeyCode, &mask);
        return ((*flag) & mask) != 0;
    }

    void SetKeyDownFlag(PRUint32 aKeyCode) {
        PRUint32 mask;
        PRUint32* flag = GetFlagWord32(aKeyCode, &mask);
        *flag |= mask;
    }

    void ClearKeyDownFlag(PRUint32 aKeyCode) {
        PRUint32 mask;
        PRUint32* flag = GetFlagWord32(aKeyCode, &mask);
        *flag &= ~mask;
    }

};

class nsChildWindow : public nsWindow {
public:
    nsChildWindow();
    ~nsChildWindow();
};

#endif 

