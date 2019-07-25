







































#ifndef __nsWindow_h__
#define __nsWindow_h__

#include <QKeyEvent>
#include <qgraphicswidget.h>
#include <QTime>

#include "nsAutoPtr.h"

#include "nsBaseWidget.h"
#include "nsGUIEvent.h"

#include "nsWeakReference.h"

#include "nsWidgetAtoms.h"

#ifdef MOZ_LOGGING


#define FORCE_PR_LOG

#include "prlog.h"
#include "nsTArray.h"

extern PRLogModuleInfo *gWidgetLog;
extern PRLogModuleInfo *gWidgetFocusLog;
extern PRLogModuleInfo *gWidgetIMLog;
extern PRLogModuleInfo *gWidgetDrawLog;

#define LOG(args) PR_LOG(gWidgetLog, 4, args)
#define LOGFOCUS(args) PR_LOG(gWidgetFocusLog, 4, args)
#define LOGIM(args) PR_LOG(gWidgetIMLog, 4, args)
#define LOGDRAW(args) PR_LOG(gWidgetDrawLog, 4, args)

#else

#ifdef DEBUG_WIDGETS

#define PR_LOG2(_args)         \
    PR_BEGIN_MACRO             \
      qDebug _args;            \
    PR_END_MACRO

#define LOG(args) PR_LOG2(args)
#define LOGFOCUS(args) PR_LOG2(args)
#define LOGIM(args) PR_LOG2(args)
#define LOGDRAW(args) PR_LOG2(args)

#else

#define LOG(args)
#define LOGFOCUS(args)
#define LOGIM(args)
#define LOGDRAW(args)

#endif

#endif 

class QEvent;
class QGraphicsView;

class MozQWidget;

class nsIdleService;

class nsWindow : public nsBaseWidget,
                 public nsSupportsWeakReference
{
public:
    nsWindow();
    virtual ~nsWindow();

    nsEventStatus DoPaint( QPainter* aPainter, const QStyleOptionGraphicsItem * aOption, QWidget* aWidget);

    static void ReleaseGlobals();

    NS_DECL_ISUPPORTS_INHERITED

    
    
    

    NS_IMETHOD         ConfigureChildren(const nsTArray<nsIWidget::Configuration>&);

    NS_IMETHOD         Create(nsIWidget        *aParent,
                              nsNativeWidget   aNativeParent,
                              const nsIntRect  &aRect,
                              EVENT_CALLBACK   aHandleEventFunction,
                              nsDeviceContext *aContext,
                              nsIAppShell      *aAppShell,
                              nsIToolkit       *aToolkit,
                              nsWidgetInitData *aInitData);

    virtual already_AddRefed<nsIWidget>
    CreateChild(const nsIntRect&  aRect,
                EVENT_CALLBACK    aHandleEventFunction,
                nsDeviceContext* aContext,
                nsIAppShell*      aAppShell = nsnull,
                nsIToolkit*       aToolkit = nsnull,
                nsWidgetInitData* aInitData = nsnull,
                PRBool            aForceUseIWidgetParent = PR_TRUE);

    NS_IMETHOD         Destroy(void);
    NS_IMETHOD         SetParent(nsIWidget* aNewParent);
    virtual nsIWidget *GetParent(void);
    virtual float      GetDPI();
    NS_IMETHOD         Show(PRBool aState);
    NS_IMETHOD         SetModal(PRBool aModal);
    NS_IMETHOD         IsVisible(PRBool & aState);
    NS_IMETHOD         ConstrainPosition(PRBool aAllowSlop,
                                         PRInt32 *aX,
                                         PRInt32 *aY);
    NS_IMETHOD         Move(PRInt32 aX,
                            PRInt32 aY);
    NS_IMETHOD         Resize(PRInt32 aWidth,
                              PRInt32 aHeight,
                              PRBool  aRepaint);
    NS_IMETHOD         Resize(PRInt32 aX,
                              PRInt32 aY,
                              PRInt32 aWidth,
                              PRInt32 aHeight,
                              PRBool   aRepaint);
    NS_IMETHOD         PlaceBehind(nsTopLevelWidgetZPlacement  aPlacement,
                                   nsIWidget                  *aWidget,
                                   PRBool                      aActivate);
    NS_IMETHOD         SetSizeMode(PRInt32 aMode);
    NS_IMETHOD         Enable(PRBool aState);
    NS_IMETHOD         SetFocus(PRBool aRaise = PR_FALSE);
    NS_IMETHOD         GetScreenBounds(nsIntRect &aRect);
    NS_IMETHOD         SetForegroundColor(const nscolor &aColor);
    NS_IMETHOD         SetBackgroundColor(const nscolor &aColor);
    NS_IMETHOD         SetCursor(nsCursor aCursor);
    NS_IMETHOD         SetCursor(imgIContainer* aCursor,
                                 PRUint32 aHotspotX, PRUint32 aHotspotY);
    NS_IMETHOD         SetHasTransparentBackground(PRBool aTransparent);
    NS_IMETHOD         GetHasTransparentBackground(PRBool& aTransparent);
    NS_IMETHOD         HideWindowChrome(PRBool aShouldHide);
    NS_IMETHOD         MakeFullScreen(PRBool aFullScreen);
    NS_IMETHOD         Invalidate(const nsIntRect &aRect,
                                  PRBool        aIsSynchronous);
    NS_IMETHOD         Update();

    virtual void*      GetNativeData(PRUint32 aDataType);
    NS_IMETHOD         SetTitle(const nsAString& aTitle);
    NS_IMETHOD         SetIcon(const nsAString& aIconSpec);
    virtual nsIntPoint WidgetToScreenOffset();
    NS_IMETHOD         DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus);

    NS_IMETHOD         EnableDragDrop(PRBool aEnable);
    NS_IMETHOD         CaptureMouse(PRBool aCapture);
    NS_IMETHOD         CaptureRollupEvents(nsIRollupListener *aListener,
                                           nsIMenuRollup *aMenuRollup,
                                           PRBool aDoCapture,
                                           PRBool aConsumeRollupEvent);

    NS_IMETHOD         SetWindowClass(const nsAString& xulWinType);

    NS_IMETHOD         GetAttention(PRInt32 aCycleCount);
    NS_IMETHOD         BeginResizeDrag   (nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical);

    NS_IMETHODIMP      SetInputMode(const IMEContext& aContext);
    NS_IMETHODIMP      GetInputMode(IMEContext& aContext);

    
    
    
    void               QWidgetDestroyed();

    

    

    void DispatchActivateEvent(void);
    void DispatchDeactivateEvent(void);
    void DispatchActivateEventOnTopLevelWindow(void);
    void DispatchDeactivateEventOnTopLevelWindow(void);
    void DispatchResizeEvent(nsIntRect &aRect, nsEventStatus &aStatus);

    nsEventStatus DispatchEvent(nsGUIEvent *aEvent) {
        nsEventStatus status;
        DispatchEvent(aEvent, status);
        return status;
    }

    
    NS_IMETHOD         IsEnabled        (PRBool *aState);

    
    void OnDestroy(void);

    
    PRBool AreBoundsSane(void);

    NS_IMETHOD         ReparentNativeWidget(nsIWidget* aNewParent);

    QWidget* GetViewWidget();

protected:
    nsCOMPtr<nsIWidget> mParent;
    
    PRPackedBool        mIsTopLevel;
    
    PRPackedBool        mIsDestroyed;

    
    PRPackedBool        mIsShown;
    
    PRBool              mEnabled;
    
    
    PRBool              mPlaced;

    
    
    nsSizeMode         mLastSizeMode;

    IMEContext          mIMEContext;

    



    void Initialize(MozQWidget *widget);
    friend class nsQtEventDispatcher;
    friend class InterceptContainer;
    friend class MozQWidget;

    virtual nsEventStatus OnMoveEvent(QGraphicsSceneHoverEvent *);
    virtual nsEventStatus OnResizeEvent(QGraphicsSceneResizeEvent *);
    virtual nsEventStatus OnCloseEvent(QCloseEvent *);
    virtual nsEventStatus OnEnterNotifyEvent(QGraphicsSceneHoverEvent *);
    virtual nsEventStatus OnLeaveNotifyEvent(QGraphicsSceneHoverEvent *);
    virtual nsEventStatus OnMotionNotifyEvent(QPointF, Qt::KeyboardModifiers);
    virtual nsEventStatus OnButtonPressEvent(QGraphicsSceneMouseEvent *);
    virtual nsEventStatus OnButtonReleaseEvent(QGraphicsSceneMouseEvent *);
    virtual nsEventStatus OnMouseDoubleClickEvent(QGraphicsSceneMouseEvent *);
    virtual nsEventStatus OnFocusInEvent(QEvent *);
    virtual nsEventStatus OnFocusOutEvent(QEvent *);
    virtual nsEventStatus OnKeyPressEvent(QKeyEvent *);
    virtual nsEventStatus OnKeyReleaseEvent(QKeyEvent *);
    virtual nsEventStatus OnScrollEvent(QGraphicsSceneWheelEvent *);

    virtual nsEventStatus contextMenuEvent(QGraphicsSceneContextMenuEvent *);
    virtual nsEventStatus imComposeEvent(QInputMethodEvent *, PRBool &handled);
    virtual nsEventStatus OnDragEnter (QGraphicsSceneDragDropEvent *);
    virtual nsEventStatus OnDragMotionEvent(QGraphicsSceneDragDropEvent *);
    virtual nsEventStatus OnDragLeaveEvent(QGraphicsSceneDragDropEvent *);
    virtual nsEventStatus OnDragDropEvent (QGraphicsSceneDragDropEvent *);
    virtual nsEventStatus showEvent(QShowEvent *);
    virtual nsEventStatus hideEvent(QHideEvent *);


#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    virtual nsEventStatus OnTouchEvent(QTouchEvent *event, PRBool &handled);

    virtual nsEventStatus OnGestureEvent(QGestureEvent *event, PRBool &handled);
    nsEventStatus DispatchGestureEvent(PRUint32 aMsg, PRUint32 aDirection,
                                       double aDelta, const nsIntPoint& aRefPoint);

    double DistanceBetweenPoints(const QPointF &aFirstPoint, const QPointF &aSecondPoint);
#endif

    void               NativeResize(PRInt32 aWidth,
                                    PRInt32 aHeight,
                                    PRBool  aRepaint);

    void               NativeResize(PRInt32 aX,
                                    PRInt32 aY,
                                    PRInt32 aWidth,
                                    PRInt32 aHeight,
                                    PRBool  aRepaint);

    void               NativeShow  (PRBool  aAction);

    enum PluginType {
        PluginType_NONE = 0,   
        PluginType_XEMBED,     
        PluginType_NONXEMBED   
    };

    void               SetPluginType(PluginType aPluginType);

    void               ThemeChanged(void);

    gfxASurface*       GetThebesSurface();

private:
    typedef struct {
        QPointF centerPoint;
        QPointF touchPoint;
        double delta;
        bool needDispatch;
        double startDistance;
        double prevDistance;
    } MozCachedTouchEvent;

    typedef struct {
        QPointF pos;
        Qt::KeyboardModifiers modifiers;
        bool needDispatch;
    } MozCachedMoveEvent;

    void*              SetupPluginPort(void);
    nsresult           SetWindowIconList(const nsTArray<nsCString> &aIconList);
    void               SetDefaultIcon(void);
    void               InitButtonEvent(nsMouseEvent &event, QGraphicsSceneMouseEvent *aEvent, int aClickCount = 1);
    PRBool             DispatchCommandEvent(nsIAtom* aCommand);
    MozQWidget*        createQWidget(MozQWidget *parent, nsWidgetInitData *aInitData);

    PRBool             IsAcceleratedQView(QGraphicsView* aView);

    MozQWidget*        mWidget;

    PRUint32           mIsVisible : 1,
                       mActivatePending : 1;
    PRInt32            mSizeState;
    PluginType         mPluginType;

    nsRefPtr<gfxASurface> mThebesSurface;
    nsCOMPtr<nsIdleService> mIdleService;

    PRBool       mIsTransparent;
 
    
    
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

    
    
    void UserActivity();

    inline void ProcessMotionEvent() {
        if (mPinchEvent.needDispatch) {
            double distance = DistanceBetweenPoints(mPinchEvent.centerPoint, mPinchEvent.touchPoint);
            distance *= 2;
            mPinchEvent.delta = distance - mPinchEvent.prevDistance;
            nsIntPoint centerPoint(mPinchEvent.centerPoint.x(), mPinchEvent.centerPoint.y());
            DispatchGestureEvent(NS_SIMPLE_GESTURE_MAGNIFY_UPDATE,
                                 0, mPinchEvent.delta, centerPoint);
            mPinchEvent.prevDistance = distance;
        }
        if (mMoveEvent.needDispatch) {
            nsMouseEvent event(PR_TRUE, NS_MOUSE_MOVE, this, nsMouseEvent::eReal);

            event.refPoint.x = nscoord(mMoveEvent.pos.x());
            event.refPoint.y = nscoord(mMoveEvent.pos.y());

            event.isShift         = ((mMoveEvent.modifiers & Qt::ShiftModifier) != 0);
            event.isControl       = ((mMoveEvent.modifiers & Qt::ControlModifier) != 0);
            event.isAlt           = ((mMoveEvent.modifiers & Qt::AltModifier) != 0);
            event.isMeta          = ((mMoveEvent.modifiers & Qt::MetaModifier) != 0);
            event.clickCount      = 0;

            DispatchEvent(&event);
            mMoveEvent.needDispatch = false;
        }

        mTimerStarted = PR_FALSE;
    }

    void DispatchMotionToMainThread() {
        if (!mTimerStarted) {
            nsCOMPtr<nsIRunnable> event =
                NS_NewRunnableMethod(this, &nsWindow::ProcessMotionEvent);
            NS_DispatchToMainThread(event);
            mTimerStarted = PR_TRUE;
        }
    }

    
    QRegion mDirtyScrollArea;

#if (QT_VERSION >= QT_VERSION_CHECK(4, 6, 0))
    QTime mLastMultiTouchTime;
#endif

    PRPackedBool mNeedsResize;
    PRPackedBool mNeedsMove;
    PRPackedBool mListenForResizes;
    PRPackedBool mNeedsShow;
    PRPackedBool mGesturesCancelled;
    MozCachedTouchEvent mPinchEvent;
    MozCachedMoveEvent mMoveEvent;
    PRPackedBool mTimerStarted;
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

