



































#ifndef nsWindow_h__
#define nsWindow_h__

#include "nsISupports.h"

#include "nsWidget.h"

#include "nsString.h"

#include "nsIDragService.h"

#include "gtkmozarea.h"
#include "gdksuperwin.h"

#ifdef USE_XIM
#include "pldhash.h"
class nsIMEGtkIC;
class nsIMEPreedit;
#endif 

class nsFont;
class nsIAppShell;





class nsWindow : public nsWidget
{

public:
  

  nsWindow();
  virtual ~nsWindow();

  static void ReleaseGlobals();

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD           WidgetToScreen(const nsRect &aOldRect, nsRect &aNewRect);

  NS_IMETHOD           PreCreateWidget(nsWidgetInitData *aWidgetInitData);

  virtual void*        GetNativeData(PRUint32 aDataType);

  NS_IMETHOD           Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
  NS_IMETHOD           ScrollWidgets(PRInt32 aDx, PRInt32 aDy);
  NS_IMETHOD           ScrollRect(nsRect &aSrcRect, PRInt32 aDx, PRInt32 aDy);

  NS_IMETHOD           SetTitle(const nsAString& aTitle);
  NS_IMETHOD           Show(PRBool aShow);
  NS_IMETHOD           CaptureMouse(PRBool aCapture);

  NS_IMETHOD           ConstrainPosition(PRBool aAllowSlop,
                                         PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD           Move(PRInt32 aX, PRInt32 aY);

  NS_IMETHOD           Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD           Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                              PRInt32 aHeight, PRBool aRepaint);

  NS_IMETHOD           BeginResizingChildren(void);
  NS_IMETHOD           EndResizingChildren(void);

  NS_IMETHOD           GetScreenBounds(nsRect &aRect);

  NS_IMETHOD           CaptureRollupEvents(nsIRollupListener * aListener,
                                           PRBool aDoCapture,
                                           PRBool aConsumeRollupEvent);
  NS_IMETHOD           Validate();
  NS_IMETHOD           Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD           Invalidate(const nsRect &aRect, PRBool aIsSynchronous);
  NS_IMETHOD           InvalidateRegion(const nsIRegion* aRegion, PRBool aIsSynchronous);
  NS_IMETHOD           SetBackgroundColor(const nscolor &aColor);
  NS_IMETHOD           SetCursor(nsCursor aCursor);
  NS_IMETHOD           Enable (PRBool aState);
  NS_IMETHOD           IsEnabled (PRBool *aState);
  NS_IMETHOD           SetFocus(PRBool aRaise);
  NS_IMETHOD           GetAttention(PRInt32 aCycleCount);
  NS_IMETHOD           Destroy();
  NS_IMETHOD           SetParent(nsIWidget* aNewParent);
  void                 ResizeTransparencyBitmap(PRInt32 aNewWidth, PRInt32 aNewHeight);
  void                 ApplyTransparencyBitmap();
#ifdef MOZ_XUL
  NS_IMETHOD           SetWindowTranslucency(PRBool aTransparent);
  NS_IMETHOD           GetWindowTranslucency(PRBool& aTransparent);
  NS_IMETHOD           UpdateTranslucentWindowAlpha(const nsRect& aRect, PRUint8* aAlphas);
  NS_IMETHOD           HideWindowChrome(PRBool aShouldHide);
  NS_IMETHOD           MakeFullScreen(PRBool aFullScreen);
#endif
  NS_IMETHOD           SetIcon(const nsAString& aIcon);
  GdkCursor           *GtkCreateCursor(nsCursor aCursorType);
  virtual void         LoseFocus(void);

  
  NS_IMETHOD           ResetInputState();

  void                 QueueDraw();
  void                 UnqueueDraw();
  void                 DoPaint(nsIRegion *aClipRegion);
  static gboolean      UpdateIdle (gpointer data);
  
  virtual GtkWindow   *GetTopLevelWindow(void);
  NS_IMETHOD           Update(void);
  virtual void         OnMotionNotifySignal(GdkEventMotion *aGdkMotionEvent);
  virtual void         OnEnterNotifySignal(GdkEventCrossing *aGdkCrossingEvent);
  virtual void         OnLeaveNotifySignal(GdkEventCrossing *aGdkCrossingEvent);
  virtual void         OnButtonPressSignal(GdkEventButton *aGdkButtonEvent);
  virtual void         OnButtonReleaseSignal(GdkEventButton *aGdkButtonEvent);
  virtual void         OnFocusInSignal(GdkEventFocus * aGdkFocusEvent);
  virtual void         OnFocusOutSignal(GdkEventFocus * aGdkFocusEvent);
  virtual void         InstallFocusInSignal(GtkWidget * aWidget);
  virtual void         InstallFocusOutSignal(GtkWidget * aWidget);
  void                 HandleGDKEvent(GdkEvent *event);

  gint                 ConvertBorderStyles(nsBorderStyle bs);

  void                 InvalidateWindowPos(void);


  
  
  
  void HandleXlibConfigureNotifyEvent(XEvent *event);

  
  virtual GtkWidget *GetOwningWidget();

  
  nsWindow *GetOwningWindow();

  
  
  nsWindowType GetOwningWindowType();

  
  virtual GdkWindow * GetRenderWindow(GtkObject * aGtkWidget);

  
  
  virtual void DestroyNative(void);

  
  PRBool GrabInProgress(void);
  
  static PRBool DragInProgress(void);

  
  

  
  
  static nsWindow *GetGrabWindow(void);
  GdkWindow *GetGdkGrabWindow(void);
  
  
  virtual GdkWindow* GetLayeringWindow();

  virtual void DispatchSetFocusEvent(void);
  virtual void DispatchLostFocusEvent(void);
  virtual void DispatchActivateEvent(void);
  virtual void DispatchDeactivateEvent(void);

  PRBool mBlockMozAreaFocusIn;

  void HandleMozAreaFocusIn(void);
  void HandleMozAreaFocusOut(void);

  
  static PRBool IconEntryMatches(PLDHashTable* aTable,
                                 const PLDHashEntryHdr* aHdr,
                                 const void* aKey);
  static void ClearIconEntry(PLDHashTable* aTable, PLDHashEntryHdr* aHdr);

protected:
  virtual void ResetInternalVisibility();
  virtual void SetInternalVisibility(PRBool aVisible);

  virtual void OnRealize(GtkWidget *aWidget);

  virtual void OnDestroySignal(GtkWidget* aGtkWidget);

  

  virtual void InitCallbacks(char * aName = nsnull);
  NS_IMETHOD CreateNative(GtkObject *parentWidget);

  PRBool      mIsTooSmall;

  GtkWidget *mShell;  
  GdkSuperWin *mSuperWin;
  GtkWidget   *mMozArea;
  GtkWidget   *mMozAreaClosestParent;

  PRBool      GetWindowPos(nscoord &x, nscoord &y);
  nscoord     mCachedX, mCachedY; 

  
  static PRBool      sIsGrabbing;
  static nsWindow   *sGrabWindow;
  static PRBool      sIsDraggingOutOf;

  
  static GHashTable *mWindowLookupTable;

  
  
  static Window     GetInnerMostWindow(Window aOriginWindow,
                                       Window aWindow,
                                       nscoord x, nscoord y,
                                       nscoord *retx, nscoord *rety,
                                       int depth);
  
  static nsWindow  *GetnsWindowFromXWindow(Window aWindow);

  static gint ClientEventSignal(GtkWidget* widget, GdkEventClient* event, void* data);
  virtual void ThemeChanged();

  

  
  static nsWindow  *mLastDragMotionWindow;
  static GdkCursor *gsGtkCursorCache[eCursorCount];

  void   InitDragEvent(nsMouseEvent &aEvent);
  void   UpdateDragStatus(nsMouseEvent &aEvent,
                          GdkDragContext *aDragContext,
                          nsIDragService *aDragService);

  
  
  static gint DragMotionSignal (GtkWidget *      aWidget,
                                GdkDragContext   *aDragContext,
                                gint             aX,
                                gint             aY,
                                guint            aTime,
                                void             *aData);
  gint OnDragMotionSignal      (GtkWidget *      aWidget,
                                GdkDragContext   *aDragContext,
                                gint             aX,
                                gint             aY,
                                guint            aTime,
                                void             *aData);

  static void DragLeaveSignal  (GtkWidget *      aWidget,
                                GdkDragContext   *aDragContext,
                                guint            aTime,
                                gpointer         aData);
  void OnDragLeaveSignal       (GtkWidget *      aWidget,
                                GdkDragContext   *aDragContext,
                                guint            aTime,
                                gpointer         aData);

  
  static gint DragDropSignal   (GtkWidget        *aWidget,
                                GdkDragContext   *aDragContext,
                                gint             aX,
                                gint             aY,
                                guint            aTime,
                                void             *aData);
  gint OnDragDropSignal        (GtkWidget        *aWidget,
                                GdkDragContext   *aDragContext,
                                gint             aX,
                                gint             aY,
                                guint            aTime,
                                void             *aData);
  
  static void DragDataReceived (GtkWidget         *aWidget,
                                GdkDragContext    *aDragContext,
                                gint               aX,
                                gint               aY,
                                GtkSelectionData  *aSelectionData,
                                guint              aInfo,
                                guint32            aTime,
                                gpointer           aData);
  void OnDragDataReceived      (GtkWidget         *aWidget,
                                GdkDragContext    *aDragContext,
                                gint               aX,
                                gint               aY,
                                GtkSelectionData  *aSelectionData,
                                guint              aInfo,
                                guint32            aTime,
                                gpointer           aData);

  
  

  void OnDragLeave(void);
  void OnDragEnter(nscoord aX, nscoord aY);

  
  
  GtkWidget         *mDragMotionWidget;
  GdkDragContext    *mDragMotionContext;
  gint               mDragMotionX;
  gint               mDragMotionY;
  guint              mDragMotionTime;
  guint              mDragMotionTimerID;
  nsCOMPtr<nsITimer> mDragLeaveTimer;

  void        ResetDragMotionTimer    (GtkWidget      *aWidget,
                                       GdkDragContext *aDragContext,
                                       gint           aX,
                                       gint           aY,
                                       guint          aTime);
  void        FireDragMotionTimer     (void);
  void        FireDragLeaveTimer      (void);
  static guint DragMotionTimerCallback (gpointer aClosure);
  static void  DragLeaveTimerCallback  (nsITimer *aTimer, void *aClosure);

  
  static PLDHashTable* sIconCache;

#ifdef USE_XIM
protected:
  static GdkFont      *gPreeditFontset;
  static GdkFont      *gStatusFontset;
  static GdkIMStyle   gInputStyle;
  static PLDHashTable gXICLookupTable;
  PRPackedBool        mIMEEnable;
  PRPackedBool        mIMECallComposeStart;
  PRPackedBool        mIMECallComposeEnd;
  PRPackedBool        mIMEIsBeingActivate;
  nsWindow*           mIMEShellWindow;
  void                SetXICSpotLocation(nsIMEGtkIC* aXIC, nsPoint aPoint);
  void                SetXICBaseFontSize(nsIMEGtkIC* aXIC, int height);
  nsCOMPtr<nsITimer>  mICSpotTimer;
  static void         ICSpotCallback(nsITimer* aTimer, void* aClosure);
  nsresult            KillICSpotTimer();
  nsresult            PrimeICSpotTimer();
  nsresult            UpdateICSpot(nsIMEGtkIC* aXIC);
  int                 mXICFontSize;

public:
  nsIMEGtkIC*        IMEGetInputContext(PRBool aCreate);

  void        ime_preedit_start();
  void        ime_preedit_draw(nsIMEGtkIC* aXIC);
  void        ime_preedit_done();
  void        ime_status_draw();

  void         IMEUnsetFocusWindow();
  void         IMESetFocusWindow();
  void         IMEGetShellWindow();
  void         IMEDestroyIC();
  void         IMEBeingActivate(PRBool aActive);
#endif 

protected:
  void              IMEComposeStart(guint aTime);
  void              IMEComposeText(GdkEventKey*,
                             const PRUnichar *aText,
                             const PRInt32 aLen,
                             const char *aFeedback);
  void              IMEComposeEnd(guint aTime);

public:
  virtual void IMECommitEvent(GdkEventKey *aEvent);

private:
  PRUnichar*   mIMECompositionUniString;
  PRInt32      mIMECompositionUniStringSize;

  nsresult     SetMiniIcon(GdkPixmap *window_pixmap,
                           GdkBitmap *window_mask);
  nsresult     SetIcon(GdkPixmap *window_pixmap, 
                       GdkBitmap *window_mask);

  void         NativeGrab(PRBool aGrab);

  PRPackedBool mLastGrabFailed;
  PRPackedBool mIsUpdating;
  PRPackedBool mLeavePending;
  PRPackedBool mRestoreFocus;

  



  PRPackedBool mHasAnonymousChildren;
  PRPackedBool mIsTranslucent;

  
  
  
  gchar*       mTransparencyBitmap;

  void DestroyNativeChildren(void);

  GtkWindow *mTransientParent;
};




class ChildWindow : public nsWindow {
public:
  ChildWindow();
  ~ChildWindow();
};

#endif 
