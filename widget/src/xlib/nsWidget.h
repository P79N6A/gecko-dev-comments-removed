








































#ifndef nsWidget_h__
#define nsWidget_h__

#include "nsBaseWidget.h"
#include "nsWeakReference.h"
#include "nsHashtable.h"
#include "prlog.h"
#include "nsIRegion.h"
#include "nsIRollupListener.h"
#include "xlibrgb.h"

#ifdef DEBUG_blizzard
#define XLIB_WIDGET_NOISY
#endif

class nsWidget : public nsBaseWidget, public nsSupportsWeakReference
{
public:
  nsWidget();
  virtual ~nsWidget();

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD Create(nsIWidget *aParent,
                    const nsRect &aRect,
                    EVENT_CALLBACK aHandleEventFunction,
                    nsIDeviceContext *aContext,
                    nsIAppShell *aAppShell = nsnull,
                    nsIToolkit *aToolkit = nsnull,
                    nsWidgetInitData *aInitData = nsnull);

  NS_IMETHOD Create(nsNativeWidget aParent,
                    const nsRect &aRect,
                    EVENT_CALLBACK aHandleEventFunction,
                    nsIDeviceContext *aContext,
                    nsIAppShell *aAppShell = nsnull,
                    nsIToolkit *aToolkit = nsnull,
                    nsWidgetInitData *aInitData = nsnull);

  virtual nsresult StandardWidgetCreate(nsIWidget *aParent,
                                        const nsRect &aRect,
                                        EVENT_CALLBACK aHandleEventFunction,
                                        nsIDeviceContext *aContext,
                                        nsIAppShell *aAppShell,
                                        nsIToolkit *aToolkit,
                                        nsWidgetInitData *aInitData,
                                        nsNativeWidget aNativeParent = nsnull);
  NS_IMETHOD Destroy();
  virtual nsIWidget *GetParent(void);
  NS_IMETHOD Show(PRBool bState);
  NS_IMETHOD IsVisible(PRBool &aState);

  NS_IMETHOD ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY);
  NS_IMETHOD Move(PRInt32 aX, PRInt32 aY);
  NS_IMETHOD Resize(PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool   aRepaint);
  NS_IMETHOD Resize(PRInt32 aX,
                    PRInt32 aY,
                    PRInt32 aWidth,
                    PRInt32 aHeight,
                    PRBool   aRepaint);

  NS_IMETHOD Enable(PRBool aState);
  NS_IMETHOD IsEnabled(PRBool *aState);
  NS_IMETHOD              SetFocus(PRBool aRaise = PR_FALSE);
  NS_IMETHOD              SetName(const char * aName);
  NS_IMETHOD              SetBackgroundColor(const nscolor &aColor);
  virtual nsIFontMetrics* GetFont(void);
  NS_IMETHOD              SetFont(const nsFont &aFont);
  NS_IMETHOD              SetCursor(nsCursor aCursor);
  void                    LockCursor(PRBool aLock);

  NS_IMETHOD Invalidate(PRBool aIsSynchronous);
  NS_IMETHOD              Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
  NS_IMETHOD              Update();
  virtual void*           GetNativeData(PRUint32 aDataType);
  NS_IMETHOD              SetColorMap(nsColorMap *aColorMap);
  NS_IMETHOD              Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
  NS_IMETHOD              SetMenuBar(nsIMenuBar * aMenuBar); 
  NS_IMETHOD              ShowMenuBar(PRBool aShow);
  NS_IMETHOD              SetTooltips(PRUint32 aNumberOfTips,nsRect* aTooltipAreas[]);   
  NS_IMETHOD              RemoveTooltips();
  NS_IMETHOD              UpdateTooltips(nsRect* aNewTips[]);
  NS_IMETHOD              WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD              ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
  NS_IMETHOD              BeginResizingChildren(void);
  NS_IMETHOD              EndResizingChildren(void);
  NS_IMETHOD              GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight);
  NS_IMETHOD              SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight);
  NS_IMETHOD              DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus);
  NS_IMETHOD              PreCreateWidget(nsWidgetInitData *aInitData);
  NS_IMETHOD              SetBounds(const nsRect &aRect);
  NS_IMETHOD              GetRequestedBounds(nsRect &aRect);

  NS_IMETHOD              CaptureRollupEvents(nsIRollupListener *aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
  NS_IMETHOD              SetTitle(const nsAString& title);
#ifdef DEBUG
  void                    DebugPrintEvent(nsGUIEvent & aEvent,Window aWindow);
#endif

  virtual PRBool          OnPaint(nsPaintEvent &event);
  virtual PRBool          OnResize(nsSizeEvent &event);
  virtual PRBool          OnDeleteWindow(void);

  
  virtual void            OnDestroy(void);
  virtual PRBool          DispatchMouseEvent(nsMouseEvent &aEvent);
  virtual PRBool          DispatchKeyEvent(nsKeyEvent &aKeyEvent);
  virtual PRBool          DispatchDestroyEvent(void);

  static nsWidget        * GetWidgetForWindow(Window aWindow);
  void                     SetVisibility(int aState); 
  void                     SetIonic(PRBool isIonic);
  static Window            GetFocusWindow(void);

  Cursor                   XlibCreateCursor(nsCursor aCursorType);

  PRBool DispatchWindowEvent(nsGUIEvent & aEvent);

  
  static Atom   WMDeleteWindow;
  static Atom   WMTakeFocus;
  static Atom   WMSaveYourself;
  static PRBool WMProtocolsInitialized;

  
  
  void *CheckParent(long ThisWindow);

  
  PRBool IsMouseInWindow(Window window, PRInt32 inMouseX, PRInt32 inMouseY);
  PRBool HandlePopup( PRInt32 inMouseX, PRInt32 inMouseY);

  void WidgetShow       (nsWidget *aWidget);
protected:

  nsCOMPtr<nsIRegion> mUpdateArea;
  
  static PRBool ConvertStatus(nsEventStatus aStatus)
                { return aStatus == nsEventStatus_eConsumeNoDefault; }

  
  virtual void CreateNativeWindow(Window aParent, nsRect aRect,
                                  XSetWindowAttributes aAttr, unsigned long aMask);
  virtual void CreateNative(Window aParent, nsRect aRect);
  virtual void DestroyNative();
  static void  DestroyNativeChildren(Display *aDisplay, Window aWindow);
  void         Map(void);
  void         Unmap(void);

  
  virtual long GetEventMask();

  
  static void  AddWindowCallback   (Window aWindow, nsWidget *aWidget);
  static void  DeleteWindowCallback(Window aWindow);

  
  void          SetUpWMHints(void);

  
  
  
  void WidgetPut        (nsWidget *aWidget);
  void WidgetMove       (nsWidget *aWidget);
  void WidgetMoveResize (nsWidget *aWidget);
  void WidgetResize     (nsWidget *aWidget);
  
  PRBool WidgetVisible  (nsRect   &aBounds);

  PRBool         mIsShown;
  int            mVisibility; 
  PRUint32       mPreferredWidth;
  PRUint32       mPreferredHeight;

  nsIWidget   *  mParentWidget;
  Window         mParentWindow;

  
  XlibRgbHandle *mXlibRgbHandle;
  Display *      mDisplay;
  Screen *       mScreen;
  Window         mBaseWindow;
  Visual *       mVisual;
  int            mDepth;
  unsigned long  mBackgroundPixel;
  PRUint32       mBorderRGB;
  unsigned long  mBorderPixel;
  nsString       mName;           
  nsRect         mRequestedSize;
  PRPackedBool   mIsToplevel;
  PRPackedBool   mMapped;
  PRPackedBool   mLastGrabFailed;

  static Window  mFocusWindow;

  
protected:
  PRBool       mListenForResizes;     
  static       nsHashtable *          gsWindowList;
  static       Cursor                 gsXlibCursorCache[eCursorCount];

  
  static nsCOMPtr<nsIRollupListener> gRollupListener;
  static nsWeakPtr                   gRollupWidget;
  static PRBool                      gRollupConsumeRollupEvent;
};

extern PRLogModuleInfo *XlibWidgetsLM;
extern PRLogModuleInfo *XlibScrollingLM;

#endif 



