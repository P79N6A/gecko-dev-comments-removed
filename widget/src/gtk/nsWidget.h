




































#ifndef nsWidget_h__
#define nsWidget_h__

#include "nsBaseWidget.h"
#include "nsWeakReference.h"
#include "nsIKBStateControl.h"
#include "nsIRegion.h"
#include "nsIRollupListener.h"

class nsILookAndFeel;
class nsIAppShell;
class nsIToolkit;

#include <gtk/gtk.h>

#include <gdk/gdkprivate.h>

#include "gtkmozbox.h"
#include "nsITimer.h"

#define NSRECT_TO_GDKRECT(ns,gdk) \
  PR_BEGIN_MACRO \
  gdk.x = ns.x; \
  gdk.y = ns.y; \
  gdk.width = ns.width; \
  gdk.height = ns.height; \
  PR_END_MACRO

#define NSCOLOR_TO_GDKCOLOR(n,g) \
  PR_BEGIN_MACRO \
  g.red = 256 * NS_GET_R(n); \
  g.green = 256 * NS_GET_G(n); \
  g.blue = 256 * NS_GET_B(n); \
  PR_END_MACRO






class nsWidget : public nsBaseWidget, public nsIKBStateControl, public nsSupportsWeakReference
{
public:
  nsWidget();
  virtual ~nsWidget();

  NS_DECL_ISUPPORTS_INHERITED

  

  NS_IMETHOD            Create(nsIWidget *aParent,
                               const nsRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell = nsnull,
                               nsIToolkit *aToolkit = nsnull,
                               nsWidgetInitData *aInitData = nsnull);
  NS_IMETHOD            Create(nsNativeWidget aParent,
                               const nsRect &aRect,
                               EVENT_CALLBACK aHandleEventFunction,
                               nsIDeviceContext *aContext,
                               nsIAppShell *aAppShell = nsnull,
                               nsIToolkit *aToolkit = nsnull,
                               nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD Destroy(void);
  nsIWidget* GetParent(void);
  virtual void OnDestroy();

  NS_IMETHOD Show(PRBool state);
  NS_IMETHOD CaptureRollupEvents(nsIRollupListener *aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD IsVisible(PRBool &aState);

  NS_IMETHOD ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD Resize(PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
  NS_IMETHOD Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                    PRInt32 aHeight, PRBool aRepaint);

  NS_IMETHOD Enable(PRBool aState);
  NS_IMETHOD IsEnabled(PRBool *aState);
  NS_IMETHOD SetFocus(PRBool aRaise);

  virtual void LoseFocus(void);

  PRBool OnResize(nsSizeEvent *event);
  virtual PRBool OnResize(nsRect &aRect);
  virtual PRBool OnMove(PRInt32 aX, PRInt32 aY);

  NS_IMETHOD SetZIndex(PRInt32 aZIndex);

  nsIFontMetrics *GetFont(void);
  NS_IMETHOD SetFont(const nsFont &aFont);

  NS_IMETHOD SetBackgroundColor(const nscolor &aColor);

  NS_IMETHOD SetCursor(nsCursor aCursor);

  NS_IMETHOD SetColorMap(nsColorMap *aColorMap);

  void* GetNativeData(PRUint32 aDataType);

  NS_IMETHOD WidgetToScreen(const nsRect &aOldRect, nsRect &aNewRect);
  NS_IMETHOD ScreenToWidget(const nsRect &aOldRect, nsRect &aNewRect);

  NS_IMETHOD BeginResizingChildren(void);
  NS_IMETHOD EndResizingChildren(void);

  NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
  NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);

  
  NS_IMETHOD SetTitle(const nsAString& aTitle);


  virtual void ConvertToDeviceCoordinates(nscoord &aX, nscoord &aY);

  
  NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect) { return NS_ERROR_FAILURE; }
  NS_IMETHOD SetMenuBar(nsIMenuBar *aMenuBar) { return NS_ERROR_FAILURE; }
  NS_IMETHOD ShowMenuBar(PRBool aShow) { return NS_ERROR_FAILURE; }
  
  NS_IMETHOD CaptureMouse(PRBool aCapture) { return NS_ERROR_FAILURE; }

  NS_IMETHOD GetWindowClass(char *aClass);
  NS_IMETHOD SetWindowClass(char *aClass);

  NS_IMETHOD Validate();
  NS_IMETHOD Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD Invalidate(const nsRect &aRect, PRBool aIsSynchronous);
  NS_IMETHOD InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);
  NS_IMETHOD Update(void);
  NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);


  
  NS_IMETHOD ResetInputState();
  NS_IMETHOD SetIMEOpenState(PRBool aState);
  NS_IMETHOD GetIMEOpenState(PRBool* aState);
  NS_IMETHOD SetIMEEnabled(PRBool aState);
  NS_IMETHOD GetIMEEnabled(PRBool* aState);
  NS_IMETHOD CancelIMEComposition();

  void InitEvent(nsGUIEvent& event, nsPoint* aPoint = nsnull);
    
  

  PRBool     ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  PRBool     DispatchMouseEvent(nsMouseEvent& aEvent);
  PRBool     DispatchStandardEvent(PRUint32 aMsg);
  PRBool     DispatchFocus(nsGUIEvent &aEvent);

  
  PRBool     mIsToplevel;

  virtual void DispatchSetFocusEvent(void);
  virtual void DispatchLostFocusEvent(void);
  virtual void DispatchActivateEvent(void);
  virtual void DispatchDeactivateEvent(void);

#ifdef DEBUG
  void IndentByDepth(FILE* out);
#endif

  
  virtual GdkWindow * GetRenderWindow(GtkObject * aGtkWidget);
  
  virtual GdkWindow* GetLayeringWindow();


  
  virtual GtkWindow *GetTopLevelWindow(void);


  PRBool   OnKey(nsKeyEvent &aEvent);
  PRBool   OnText(nsTextEvent &aEvent)       { return OnInput(aEvent); };
  PRBool   OnComposition(nsCompositionEvent &aEvent) { return OnInput(aEvent); };
  PRBool   OnInput(nsInputEvent &aEvent);

  
  virtual GtkWidget *GetOwningWidget();

  
  static void SetLastEventTime(guint32 aTime);
  static void GetLastEventTime(guint32 *aTime);

  static void DropMotionTarget(void);

  
  
  static void DragStarted(void) { DropMotionTarget(); };

  virtual void ThemeChanged();

protected:

  virtual void InitCallbacks(char * aName = nsnull);

  NS_IMETHOD CreateNative(GtkObject *parentWindow) { return NS_OK; }

  nsresult CreateWidget(nsIWidget *aParent,
                        const nsRect &aRect,
                        EVENT_CALLBACK aHandleEventFunction,
                        nsIDeviceContext *aContext,
                        nsIAppShell *aAppShell,
                        nsIToolkit *aToolkit,
                        nsWidgetInitData *aInitData,
                        nsNativeWidget aNativeParent = nsnull);


  PRBool DispatchWindowEvent(nsGUIEvent* event);

  
  
  virtual void DestroyNative(void);

  
  virtual void SetFontNative(GdkFont *aFont);
  
  virtual void SetBackgroundColorNative(GdkColor *aColorNor,
                                        GdkColor *aColorBri,
                                        GdkColor *aColorDark);

  
  PRBool       mHasFocus;


  
  
public:
  
  static nsWidget *sFocusWindow;

protected:
  

  
  
  
  
  
  void InstallEnterNotifySignal(GtkWidget * aWidget);

  void InstallLeaveNotifySignal(GtkWidget * aWidget);

  void InstallButtonPressSignal(GtkWidget * aWidget);

  void InstallButtonReleaseSignal(GtkWidget * aWidget);

  virtual
  void InstallFocusInSignal(GtkWidget * aWidget);

  virtual
  void InstallFocusOutSignal(GtkWidget * aWidget);

  void InstallRealizeSignal(GtkWidget * aWidget);

  void AddToEventMask(GtkWidget * aWidget,
                      gint        aEventMask);

  
  
  
  
  
  virtual void OnMotionNotifySignal(GdkEventMotion * aGdkMotionEvent);
  virtual void OnEnterNotifySignal(GdkEventCrossing * aGdkCrossingEvent);
  virtual void OnLeaveNotifySignal(GdkEventCrossing * aGdkCrossingEvent);
  virtual void OnButtonPressSignal(GdkEventButton * aGdkButtonEvent);
  virtual void OnButtonReleaseSignal(GdkEventButton * aGdkButtonEvent);
  virtual void OnFocusInSignal(GdkEventFocus * aGdkFocusEvent);
  virtual void OnFocusOutSignal(GdkEventFocus * aGdkFocusEvent);
  virtual void OnRealize(GtkWidget *aWidget);

  virtual void OnDestroySignal(GtkWidget* aGtkWidget);

  
  static gint  DestroySignal(GtkWidget *      aGtkWidget,
                             nsWidget*        aWidget);

public:
  virtual void IMECommitEvent(GdkEventKey *aEvent);

  
  
  
  
  
  
  virtual void ResetInternalVisibility();

  
  
  void ResetZOrder();

protected:
  
  
  virtual void SetInternalVisibility(PRBool aVisible);

  
  
  
  
  

  static gint EnterNotifySignal(GtkWidget *        aWidget, 
                                GdkEventCrossing * aGdkCrossingEvent, 
                                gpointer           aData);
  
  static gint LeaveNotifySignal(GtkWidget *        aWidget, 
                                GdkEventCrossing * aGdkCrossingEvent, 
                                gpointer           aData);

  static gint ButtonPressSignal(GtkWidget *      aWidget, 
                                GdkEventButton * aGdkButtonEvent, 
                                gpointer         aData);

  static gint ButtonReleaseSignal(GtkWidget *      aWidget, 
                                  GdkEventButton * aGdkButtonEvent, 
                                  gpointer         aData);

  static gint RealizeSignal(GtkWidget *      aWidget, 
                            gpointer         aData);


  static gint FocusInSignal(GtkWidget *      aWidget, 
                            GdkEventFocus *  aGdkFocusEvent, 
                            gpointer         aData);

  static gint FocusOutSignal(GtkWidget *      aWidget, 
                             GdkEventFocus *  aGdkFocusEvent, 
                             gpointer         aData);

protected:

  
  
  
  
  
  void InstallSignal(GtkWidget *   aWidget,
                     gchar *       aSignalName,
                     GtkSignalFunc aSignalFunction);
  
  void InitMouseEvent(GdkEventButton * aGdkButtonEvent,
                      nsMouseEvent &   anEvent);

#ifdef DEBUG
  nsCAutoString  debug_GetName(GtkObject * aGtkWidget);
  nsCAutoString  debug_GetName(GtkWidget * aGtkWidget);
  PRInt32       debug_GetRenderXID(GtkObject * aGtkWidget);
  PRInt32       debug_GetRenderXID(GtkWidget * aGtkWidget);
#endif

  GtkWidget *mWidget;
  
  GtkWidget *mMozBox;

  nsCOMPtr<nsIWidget> mParent;

  
  
  nsCOMPtr<nsIRegion> mUpdateArea;

  PRUint32 mPreferredWidth, mPreferredHeight;
  PRPackedBool mListenForResizes;
  PRPackedBool mShown;
  PRPackedBool mInternalShown;

  
  static nsCOMPtr<nsIRollupListener> gRollupListener;
  static nsWeakPtr                   gRollupWidget;

  
  
  static guint32 sLastEventTime;

private:
  
  
  static PRBool mGDKHandlerInstalled;
  
  
  static PRBool sTimeCBSet;

  
  
  
public:
  static nsWidget *sButtonMotionTarget;
  static gint sButtonMotionRootX;
  static gint sButtonMotionRootY;
  static gint sButtonMotionWidgetX;
  static gint sButtonMotionWidgetY;
};

#endif 
