






































#ifndef NSCOMMONWIDGET_H
#define NSCOMMONWIDGET_H

#include "nsBaseWidget.h"

#include "nsEvent.h"

#include <qevent.h> 

class nsIToolkit;
class nsWidgetInitData;
class nsIDeviceContext;
class nsIAppShell;
class nsIFontMetrics;
class nsColorMap;
class nsFont;
class nsRect;
class nsAString;
class nsIMenuBar;
class nsGUIEvent;
class nsIRollupListener;
class QWidget;
class nsQtEventDispatcher;

class nsCommonWidget : public nsBaseWidget
{
public:
    nsCommonWidget();
    ~nsCommonWidget();

    NS_DECL_ISUPPORTS_INHERITED

    NS_IMETHOD Show(PRBool);
    NS_IMETHOD IsVisible(PRBool&);

    NS_IMETHOD ConstrainPosition(PRBool, PRInt32*, PRInt32*);
    NS_IMETHOD Move(PRInt32, PRInt32);
    NS_IMETHOD Resize(PRInt32, PRInt32, PRBool);
    NS_IMETHOD Resize(PRInt32, PRInt32, PRInt32, PRInt32, PRBool);
    NS_IMETHOD Enable(PRBool);
    NS_IMETHOD IsEnabled(PRBool*);
    NS_IMETHOD SetFocus(PRBool araise = PR_FALSE);

    virtual nsIFontMetrics* GetFont();

    NS_IMETHOD SetFont(const nsFont&);
    NS_IMETHOD Invalidate(PRBool);
    NS_IMETHOD Invalidate(const nsRect&, int);
    NS_IMETHOD Update();
    NS_IMETHOD SetColorMap(nsColorMap*);
    NS_IMETHOD Scroll(PRInt32, PRInt32, nsRect*);
    NS_IMETHOD ScrollWidgets(PRInt32 aDx, PRInt32 aDy);

    NS_IMETHOD SetModal(PRBool aModal);

    virtual void* GetNativeData(PRUint32);

    NS_IMETHOD SetTitle(const nsAString&);
    NS_IMETHOD SetMenuBar(nsIMenuBar*);
    NS_IMETHOD ShowMenuBar(PRBool);
    NS_IMETHOD GetScreenBounds(nsRect &aRect);
    NS_IMETHOD WidgetToScreen(const nsRect&, nsRect&);
    NS_IMETHOD ScreenToWidget(const nsRect&, nsRect&);
    NS_IMETHOD BeginResizingChildren();
    NS_IMETHOD EndResizingChildren();
    NS_IMETHOD GetPreferredSize(PRInt32&, PRInt32&);
    NS_IMETHOD SetPreferredSize(PRInt32, PRInt32);
    NS_IMETHOD DispatchEvent(nsGUIEvent*, nsEventStatus&);
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener*, PRBool, PRBool);

    
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

    nsCursor GetCursor();
    NS_METHOD SetCursor(nsCursor aCursor);

protected:
    



    friend class nsQtEventDispatcher;
    friend class InterceptContainer;
    friend class MozQWidget;

    virtual bool mousePressEvent(QMouseEvent *);
    virtual bool mouseReleaseEvent(QMouseEvent *);
    virtual bool mouseDoubleClickEvent(QMouseEvent *);
    virtual bool mouseMoveEvent(QMouseEvent *);
    virtual bool wheelEvent(QWheelEvent *);
    virtual bool keyPressEvent(QKeyEvent *);
    virtual bool keyReleaseEvent(QKeyEvent *);
    virtual bool focusInEvent(QFocusEvent *);
    virtual bool focusOutEvent(QFocusEvent *);
    virtual bool enterEvent(QEvent *);
    virtual bool leaveEvent(QEvent *);
    virtual bool paintEvent(QPaintEvent *);
    virtual bool moveEvent(QMoveEvent *);
    virtual bool resizeEvent(QResizeEvent *);
    virtual bool closeEvent(QCloseEvent *);
    virtual bool contextMenuEvent(QContextMenuEvent *);
    virtual bool imStartEvent(QIMEvent *);
    virtual bool imComposeEvent(QIMEvent *);
    virtual bool imEndEvent(QIMEvent *);
    virtual bool dragEnterEvent(QDragEnterEvent *);
    virtual bool dragMoveEvent(QDragMoveEvent *);
    virtual bool dragLeaveEvent(QDragLeaveEvent *);
    virtual bool dropEvent(QDropEvent *);
    virtual bool showEvent(QShowEvent *);
    virtual bool hideEvent(QHideEvent *);

protected:
    virtual QWidget  *createQWidget(QWidget *parent, nsWidgetInitData *aInitData) = 0;
    virtual void NativeResize(PRInt32, PRInt32, PRInt32, PRInt32, PRBool);
    virtual void NativeResize(PRInt32, PRInt32, PRBool);
    virtual void NativeShow(PRBool);

    static bool ignoreEvent(nsEventStatus aStatus)
                { return aStatus == nsEventStatus_eConsumeNoDefault; }

    




    void Initialize(QWidget *widget);

    void DispatchGotFocusEvent(void);
    void DispatchLostFocusEvent(void);
    void DispatchActivateEvent(void);
    void DispatchDeactivateEvent(void);
    void DispatchResizeEvent(nsRect &aRect, nsEventStatus &aStatus);

    void InitKeyEvent(nsKeyEvent *nsEvent, QKeyEvent *qEvent);
    void InitMouseEvent(nsMouseEvent *nsEvent, QMouseEvent *qEvent, int aClickCount);
    void InitMouseWheelEvent(nsMouseScrollEvent *aEvent, QWheelEvent *qEvent);

    void CommonCreate(nsIWidget *aParent, PRBool aListenForResizes);

    PRBool AreBoundsSane() const;

protected:
    QWidget *mContainer;
    QWidget *mWidget;
    PRPackedBool   mListenForResizes;
    PRPackedBool   mNeedsResize;
    PRPackedBool   mNeedsShow;
    PRPackedBool   mIsShown;
    nsCOMPtr<nsIWidget> mParent;

private:
    nsresult NativeCreate(nsIWidget        *aParent,
                          QWidget          *aNativeParent,
                          const nsRect     &aRect,
                          EVENT_CALLBACK    aHandleEventFunction,
                          nsIDeviceContext *aContext,
                          nsIAppShell      *aAppShell,
                          nsIToolkit       *aToolkit,
                          nsWidgetInitData *aInitData);

};

#endif
