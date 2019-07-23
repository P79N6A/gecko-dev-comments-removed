






































#ifndef __nsWindow_h__
#define __nsWindow_h__

#include "nsAutoPtr.h"

#include "nsCommonWidget.h"

#include "nsWeakReference.h"

#include "nsIDragService.h"
#include "nsITimer.h"
#include "nsWidgetAtoms.h"


#ifdef Q_WS_X11
#include <QX11Info>
#endif

class QEvent;

class MozQWidget;

class nsWindow : public nsCommonWidget, public nsSupportsWeakReference
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
    qint32             ConvertBorderStyles(nsBorderStyle aStyle);

protected:
    



    void Initialize(QWidget *widget);
    friend class nsQtEventDispatcher;
    friend class InterceptContainer;
    friend class MozQWidget;

    virtual nsEventStatus OnExposeEvent(QPaintEvent *);
    virtual nsEventStatus OnConfigureEvent(QMoveEvent *);
    virtual nsEventStatus OnSizeAllocate(QResizeEvent *);
    virtual nsEventStatus OnDeleteEvent(QCloseEvent *);
    virtual nsEventStatus OnEnterNotifyEvent(QEvent *);
    virtual nsEventStatus OnLeaveNotifyEvent(QEvent *);
    virtual nsEventStatus OnMotionNotifyEvent(QMouseEvent *);
    virtual nsEventStatus OnButtonPressEvent(QMouseEvent *);
    virtual nsEventStatus OnButtonReleaseEvent(QMouseEvent *);
    virtual nsEventStatus mouseDoubleClickEvent(QMouseEvent *);
    virtual nsEventStatus OnContainerFocusInEvent(QFocusEvent *);
    virtual nsEventStatus OnContainerFocusOutEvent(QFocusEvent *);
    virtual nsEventStatus OnKeyPressEvent(QKeyEvent *);
    virtual nsEventStatus OnKeyReleaseEvent(QKeyEvent *);
    virtual nsEventStatus OnScrollEvent(QWheelEvent *);

    virtual nsEventStatus contextMenuEvent(QContextMenuEvent *);
    virtual nsEventStatus imStartEvent(QEvent *);
    virtual nsEventStatus imComposeEvent(QEvent *);
    virtual nsEventStatus imEndEvent(QEvent *);
    virtual nsEventStatus OnDragEnter (QDragEnterEvent *);
    virtual nsEventStatus OnDragMotionEvent(QDragMoveEvent *);
    virtual nsEventStatus OnDragLeaveEvent(QDragLeaveEvent *);
    virtual nsEventStatus OnDragDropEvent (QDropEvent *);
    virtual nsEventStatus showEvent(QShowEvent *);
    virtual nsEventStatus hideEvent(QHideEvent *);

    nsEventStatus         OnWindowStateEvent(QEvent *aEvent);

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
    void               GrabKeyboard (void);
    void               ReleaseGrabs (void);
    void               GrabPointer(void);

    enum PluginType {
        PluginType_NONE = 0,   
        PluginType_XEMBED,     
        PluginType_NONXEMBED   
    };

    void               SetPluginType(PluginType aPluginType);
    void               SetNonXEmbedPluginFocus(void);
    void               LoseNonXEmbedPluginFocus(void);

    void               ThemeChanged(void);

    NS_IMETHOD         BeginResizeDrag   (nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);

   void                ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight);
   void                ApplyTransparencyBitmap();
   NS_IMETHOD          SetHasTransparentBackground(PRBool aTransparent);
   NS_IMETHOD          GetHasTransparentBackground(PRBool& aTransparent);
   nsresult            UpdateTranslucentWindowAlphaInternal(const nsRect& aRect,
                                                            PRUint8* aAlphas, PRInt32 aStride);

   gfxASurface        *GetThebesSurface();

private:
    void               GetToplevelWidget(QWidget **aWidget);
    void               SetUrgencyHint(QWidget *top_window, PRBool state);
    void              *SetupPluginPort(void);
    nsresult           SetWindowIconList(const nsCStringArray &aIconList);
    void               SetDefaultIcon(void);
    void               InitButtonEvent(nsMouseEvent &aEvent, QMouseEvent *aEvent, int aClickCount = 1);
    PRBool             DispatchCommandEvent(nsIAtom* aCommand);
    QWidget           *createQWidget(QWidget *parent, nsWidgetInitData *aInitData);

    QWidget            *mDrawingarea;
    MozQWidget *mMozQWidget;

    PRUint32            mIsVisible : 1,
                        mRetryPointerGrab : 1,
                        mActivatePending : 1,
                        mRetryKeyboardGrab : 1;
    PRInt32             mSizeState;
    PluginType          mPluginType;

    PRInt32             mTransparencyBitmapWidth;
    PRInt32             mTransparencyBitmapHeight;

    nsRefPtr<gfxASurface> mThebesSurface;

    
    PRBool       mIsTransparent;
    
    
    
    char*       mTransparencyBitmap;
 
    
    
    void   InitDragEvent         (nsMouseEvent &aEvent);

    
    
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
    PRInt32 mQCursor;

};

class nsChildWindow : public nsWindow
{
public:
    nsChildWindow();
    ~nsChildWindow();

  PRInt32 mChildID;
};

class nsPopupWindow : public nsWindow
{
public:
  nsPopupWindow ();
  ~nsPopupWindow ();

  PRInt32 mChildID;
};
#endif 

