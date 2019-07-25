




































#ifndef NSWINDOW_H_
#define NSWINDOW_H_

#include "nsBaseWidget.h"
#include "gfxPoint.h"

#include "nsTArray.h"

class gfxASurface;
class nsIdleService;

namespace mozilla {
    class AndroidGeckoEvent;
    class AndroidKeyEvent;
}

class nsWindow :
    public nsBaseWidget
{
public:
    using nsBaseWidget::GetLayerManager;

    nsWindow();
    virtual ~nsWindow();

    NS_DECL_ISUPPORTS_INHERITED

    static void OnGlobalAndroidEvent(mozilla::AndroidGeckoEvent *ae);
    static gfxIntSize GetAndroidScreenBounds();

    nsWindow* FindWindowForPoint(const nsIntPoint& pt);

    void OnAndroidEvent(mozilla::AndroidGeckoEvent *ae);
    void OnDraw(mozilla::AndroidGeckoEvent *ae);
    void OnMotionEvent(mozilla::AndroidGeckoEvent *ae);
    void OnMultitouchEvent(mozilla::AndroidGeckoEvent *ae);
    void OnKeyEvent(mozilla::AndroidGeckoEvent *ae);
    void OnIMEEvent(mozilla::AndroidGeckoEvent *ae);

    void OnSizeChanged(const gfxIntSize& aSize);

    void InitEvent(nsGUIEvent& event, nsIntPoint* aPoint = 0);

    
    
    

    NS_IMETHOD Create(nsIWidget *aParent,
                      nsNativeWidget aNativeParent,
                      const nsIntRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData);
    NS_IMETHOD Destroy(void);
    NS_IMETHOD ConfigureChildren(const nsTArray<nsIWidget::Configuration>&);
    NS_IMETHOD SetParent(nsIWidget* aNewParent);
    virtual nsIWidget *GetParent(void);
    virtual float GetDPI();
    NS_IMETHOD Show(PRBool aState);
    NS_IMETHOD SetModal(PRBool aModal);
    NS_IMETHOD IsVisible(PRBool & aState);
    NS_IMETHOD ConstrainPosition(PRBool aAllowSlop,
                                 PRInt32 *aX,
                                 PRInt32 *aY);
    NS_IMETHOD Move(PRInt32 aX,
                    PRInt32 aY);
    NS_IMETHOD Resize(PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool  aRepaint);
    NS_IMETHOD Resize(PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool aRepaint);
    NS_IMETHOD SetZIndex(PRInt32 aZIndex);
    NS_IMETHOD PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                           nsIWidget *aWidget,
                           PRBool aActivate);
    NS_IMETHOD SetSizeMode(PRInt32 aMode);
    NS_IMETHOD Enable(PRBool aState);
    NS_IMETHOD IsEnabled(PRBool *aState);
    NS_IMETHOD Invalidate(const nsIntRect &aRect,
                          PRBool aIsSynchronous);
    NS_IMETHOD Update();
    NS_IMETHOD SetFocus(PRBool aRaise = PR_FALSE);
    NS_IMETHOD GetScreenBounds(nsIntRect &aRect);
    virtual nsIntPoint WidgetToScreenOffset();
    NS_IMETHOD DispatchEvent(nsGUIEvent *aEvent, nsEventStatus &aStatus);
    nsEventStatus DispatchEvent(nsGUIEvent *aEvent);
    NS_IMETHOD MakeFullScreen(PRBool aFullScreen);
    NS_IMETHOD SetWindowClass(const nsAString& xulWinType);



    NS_IMETHOD SetForegroundColor(const nscolor &aColor) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetBackgroundColor(const nscolor &aColor) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetCursor(nsCursor aCursor) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetCursor(imgIContainer* aCursor,
                         PRUint32 aHotspotX,
                         PRUint32 aHotspotY) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD SetHasTransparentBackground(PRBool aTransparent) { return NS_OK; }
    NS_IMETHOD GetHasTransparentBackground(PRBool& aTransparent) { aTransparent = PR_FALSE; return NS_OK; }
    NS_IMETHOD HideWindowChrome(PRBool aShouldHide) { return NS_ERROR_NOT_IMPLEMENTED; }
    virtual void* GetNativeData(PRUint32 aDataType);
    NS_IMETHOD SetTitle(const nsAString& aTitle) { return NS_OK; }
    NS_IMETHOD SetIcon(const nsAString& aIconSpec) { return NS_OK; }
    NS_IMETHOD EnableDragDrop(PRBool aEnable) { return NS_OK; }
    NS_IMETHOD CaptureMouse(PRBool aCapture) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener,
                                   nsIMenuRollup *aMenuRollup,
                                   PRBool aDoCapture,
                                   PRBool aConsumeRollupEvent) { return NS_ERROR_NOT_IMPLEMENTED; }

    NS_IMETHOD GetAttention(PRInt32 aCycleCount) { return NS_ERROR_NOT_IMPLEMENTED; }
    NS_IMETHOD BeginResizeDrag(nsGUIEvent* aEvent, PRInt32 aHorizontal, PRInt32 aVertical) { return NS_ERROR_NOT_IMPLEMENTED; }

    NS_IMETHOD ResetInputState();
    NS_IMETHOD SetInputMode(const IMEContext& aContext);
    NS_IMETHOD GetInputMode(IMEContext& aContext);
    NS_IMETHOD CancelIMEComposition();

    NS_IMETHOD OnIMEFocusChange(PRBool aFocus);
    NS_IMETHOD OnIMETextChange(PRUint32 aStart, PRUint32 aOldEnd, PRUint32 aNewEnd);
    NS_IMETHOD OnIMESelectionChange(void);
    virtual nsIMEUpdatePreference GetIMEUpdatePreference();

    LayerManager* GetLayerManager(LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                  bool* aAllowRetaining = nsnull);
    gfxASurface* GetThebesSurface();

    NS_IMETHOD ReparentNativeWidget(nsIWidget* aNewParent);
protected:
    void BringToFront();
    nsWindow *FindTopLevel();
    PRBool DrawTo(gfxASurface *targetSurface);
    PRBool IsTopLevel();
    void OnIMEAddRange(mozilla::AndroidGeckoEvent *ae);

    
    
    void UserActivity();

    PRPackedBool mIsVisible;
    nsTArray<nsWindow*> mChildren;
    nsWindow* mParent;
    nsWindow* mFocus;

    bool mGestureFinished;
    double mStartDist;
    double mLastDist;
    nsAutoPtr<nsIntPoint> mStartPoint;

    
    double mSwipeMaxPinchDelta;
    double mSwipeMinDistance;

    nsCOMPtr<nsIdleService> mIdleService;

    PRBool mIMEComposing;
    nsString mIMEComposingText;
    nsAutoTArray<nsTextRange, 4> mIMERanges;

    IMEContext mIMEContext;

    static void DumpWindows();
    static void DumpWindows(const nsTArray<nsWindow*>& wins, int indent = 0);
    static void LogWindow(nsWindow *win, int index, int indent);

private:
    void InitKeyEvent(nsKeyEvent& event, mozilla::AndroidGeckoEvent& key);
    void DispatchGestureEvent(mozilla::AndroidGeckoEvent *ae);
    void DispatchGestureEvent(PRUint32 msg, PRUint32 direction, double delta,
                               const nsIntPoint &refPoint, PRUint64 time);
    void HandleSpecialKey(mozilla::AndroidGeckoEvent *ae);
};

#endif 
