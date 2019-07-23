




















































#ifndef _nswindow_h
#define _nswindow_h

#include "nsWidgetDefs.h"
#include "nsBaseWidget.h"
#include "nsToolkit.h"
#include "nsSwitchToUIThread.h"
#include "gfxOS2Surface.h"
#include "gfxContext.h"

class nsIMenuBar;
class imgIContainer;



#ifdef DEBUG_FOCUS
  #define DEBUGFOCUS(what) printf("[%x] "#what" (%d)\n", (int)this, mWindowIdentifier)
#else
  #define DEBUGFOCUS(what)
#endif












   
   #define   nsWindowState_ePrecreate      0x00000001
   
   #define   nsWindowState_eInCreate       0x00000002
   
   #define      nsWindowState_eLive        0x00000004
   
   #define      nsWindowState_eClosing     0x00000008
   
   #define      nsWindowState_eDoingDelete 0x00000010
   
   #define      nsWindowState_eDead        0x00000100         

MRESULT EXPENTRY fnwpNSWindow( HWND, ULONG, MPARAM, MPARAM);
MRESULT EXPENTRY fnwpFrame( HWND, ULONG, MPARAM, MPARAM);

class nsWindow : public nsBaseWidget,
                 public nsSwitchToUIThread
{
 public:
   
   nsWindow();
   virtual ~nsWindow();

  static void ReleaseGlobals();

   

   
   NS_IMETHOD Create( nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell = nsnull,
                      nsIToolkit *aToolkit = nsnull,
                      nsWidgetInitData *aInitData = nsnull);
   NS_IMETHOD Create( nsNativeWidget aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell = nsnull,
                      nsIToolkit *aToolkit = nsnull,
                      nsWidgetInitData *aInitData = nsnull);
   gfxASurface* GetThebesSurface();
   NS_IMETHOD Destroy(); 

   
   virtual nsIWidget *GetParent();

    NS_IMETHOD              SetSizeMode(PRInt32 aMode);

   
   NS_IMETHOD Show( PRBool bState);
   NS_IMETHOD ConstrainPosition(PRBool aAllowSlop, PRInt32 *aX, PRInt32 *aY);
   NS_IMETHOD Move( PRInt32 aX, PRInt32 aY);
   NS_IMETHOD Resize( PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint);
   NS_IMETHOD Resize( PRInt32 aX,
                      PRInt32 aY,
                      PRInt32 aWidth,
                      PRInt32 aHeight,
                      PRBool   aRepaint);
   NS_IMETHOD GetClientBounds( nsRect &aRect);
   NS_IMETHOD Enable( PRBool aState);
   NS_IMETHOD IsEnabled(PRBool *aState);
   NS_IMETHOD SetFocus(PRBool aRaise);
   NS_IMETHOD GetBounds(nsRect &aRect);
   NS_IMETHOD IsVisible( PRBool &aState);
   NS_IMETHOD PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                          nsIWidget *aWidget, PRBool aActivate);

   NS_IMETHOD CaptureMouse(PRBool aCapture);

   NS_IMETHOD ModalEventFilter( PRBool aRealEvent, void *aEvent,
                                PRBool *aForWindow );

   NS_IMETHOD GetPreferredSize( PRInt32 &aWidth, PRInt32 &aHeight);
   NS_IMETHOD SetPreferredSize( PRInt32 aWidth, PRInt32 aHeight);

   NS_IMETHOD BeginResizingChildren();
   NS_IMETHOD EndResizingChildren();
   NS_IMETHOD WidgetToScreen( const nsRect &aOldRect, nsRect &aNewRect);
   NS_IMETHOD ScreenToWidget( const nsRect &aOldRect, nsRect &aNewRect);
   NS_IMETHOD DispatchEvent( struct nsGUIEvent *event, nsEventStatus &aStatus);
   NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);

   NS_IMETHOD              GetLastInputEventTime(PRUint32& aTime);

   
   virtual nsIFontMetrics *GetFont();
   NS_IMETHOD              SetFont( const nsFont &aFont);
   NS_IMETHOD              SetColorMap( nsColorMap *aColorMap);
   NS_IMETHOD              SetCursor( nsCursor aCursor);
   NS_IMETHOD              SetCursor(imgIContainer* aCursor,
                                     PRUint32 aHotspotX, PRUint32 aHotspotY);
   NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);
   NS_IMETHOD              SetTitle( const nsAString& aTitle); 
   NS_IMETHOD              SetIcon(const nsAString& aIconSpec); 
   NS_IMETHOD              SetMenuBar(nsIMenuBar * aMenuBar) { return NS_ERROR_FAILURE; } 
   NS_IMETHOD              ShowMenuBar(PRBool aShow)         { return NS_ERROR_FAILURE; } 
   NS_IMETHOD              Invalidate( PRBool aIsSynchronous);
   NS_IMETHOD              Invalidate( const nsRect & aRect, PRBool aIsSynchronous);
   NS_IMETHOD              InvalidateRegion(const nsIRegion *aRegion, PRBool aIsSynchronous);
   NS_IMETHOD              Update();
   NS_IMETHOD              Scroll( PRInt32 aDx, PRInt32 aDy, nsRect *aClipRect);
   NS_IMETHOD              ScrollWidgets(PRInt32 aDx, PRInt32 aDy);
   NS_IMETHOD              ScrollRect(nsRect &aRect, PRInt32 aDx, PRInt32 aDy);

   
   virtual void  *GetNativeData( PRUint32 aDataType);
   virtual void   FreeNativeData( void *aDatum, PRUint32 aDataType);
   virtual HWND   GetMainWindow() const           { return mWnd; }

   
    virtual BOOL            CallMethod(MethodInfo *info);

   
   ULONG  GetNextID()    { return mNextID++; }
   void   NS2PM_PARENT( POINTL &ptl);
   void   NS2PM( POINTL &ptl);
   void   NS2PM( RECTL &rcl);

protected:
    static  BOOL            DealWithPopups ( ULONG inMsg, MRESULT* outResult ) ;

    static  PRBool          EventIsInsideWindow(nsWindow* aWindow); 

    static  nsWindow *      GetNSWindowPtr(HWND aWnd);
    static  BOOL            SetNSWindowPtr(HWND aWnd, nsWindow * ptr);

   static  nsWindow*   gCurrentWindow;
   
   virtual PCSZ  WindowClass();
   virtual ULONG WindowStyle();

   
   virtual void     PostCreateWidget()            {}
   virtual PRInt32  GetClientHeight()             { return mBounds.height; }
   virtual ULONG    GetSWPFlags( ULONG flags)     { return flags; }
   virtual void     SetupForPrint( HWND ) {}

   
   virtual nsresult GetWindowText( nsString &str, PRUint32 *rc);
   virtual void     AddToStyle( ULONG style);
   virtual void     RemoveFromStyle( ULONG style);
   
   virtual BOOL     SetWindowPos( HWND hwndInsertBehind, long x, long y,
                                  long cx, long cy, unsigned long flags);

   
   

   
   virtual PRBool ProcessMessage( ULONG m, MPARAM p1, MPARAM p2, MRESULT &r);
   virtual PRBool OnPaint();
   virtual void   OnDestroy();
   virtual PRBool OnReposition( PSWP pNewSwp);
   virtual PRBool OnResize( PRInt32 aX, PRInt32 aY);
   virtual PRBool OnMove( PRInt32 aX, PRInt32 aY);
   virtual PRBool OnKey( MPARAM mp1, MPARAM mp2);
   virtual PRBool OnRealizePalette();
   virtual PRBool DispatchFocus( PRUint32 aEventType, PRBool isMozWindowTakingFocus);
   virtual PRBool OnScroll( ULONG msgid, MPARAM mp1, MPARAM mp2);
   virtual PRBool OnVScroll( MPARAM mp1, MPARAM mp2);
   virtual PRBool OnHScroll( MPARAM mp1, MPARAM mp2);
   virtual PRBool OnControl( MPARAM mp1, MPARAM mp2);
   
   virtual PRBool OnPresParamChanged( MPARAM mp1, MPARAM mp2);
   virtual PRBool OnDragDropMsg(ULONG msg, MPARAM mp1, MPARAM mp2, MRESULT &mr);

   static BOOL sIsRegistered;

   
   HWND      mWnd;            
   HWND      mFrameWnd;       
   PFNWP     mPrevWndProc;    
   nsWindow *mParent;         
   ULONG     mNextID;         
   PSWP      mSWPs;           
   ULONG     mlHave, mlUsed;  
   HPOINTER  mFrameIcon;      
   VDKEY     mDeadKey;        
   BOOL      mHaveDeadKey;    
   QMSG      mQmsg;
   PRBool    mIsTopWidgetWindow;
   BOOL      mIsScrollBar;
   BOOL      mInSetFocus;
   BOOL      mChromeHidden;
   nsContentType mContentType;
   HPS       mDragHps;        
   PRUint32  mDragStatus;     
   HPOINTER  mCssCursorHPtr;  
   nsCOMPtr<imgIContainer> mCssCursorImg;  

   HWND      GetParentHWND() const;
   HWND      GetHWND() const   { return mWnd; }
   PFNWP     GetPrevWP() const { return mPrevWndProc; }

   
   PRInt32        mPreferredHeight;
   PRInt32        mPreferredWidth;
   nsToolkit     *mOS2Toolkit;
   nsFont        *mFont;
   nsIMenuBar    *mMenuBar;
   PRInt32        mWindowState;
   nsRefPtr<gfxOS2Surface> mThebesSurface;

   
   void DoCreate( HWND hwndP, nsWindow *wndP, const nsRect &rect,
                  EVENT_CALLBACK aHandleEventFunction,
                  nsIDeviceContext *aContext, nsIAppShell *aAppShell,
                  nsIToolkit *aToolkit, nsWidgetInitData *aInitData);

   virtual void RealDoCreate( HWND hwndP, nsWindow *aParent,
                              const nsRect &aRect,
                              EVENT_CALLBACK aHandleEventFunction,
                              nsIDeviceContext *aContext,
                              nsIAppShell *aAppShell,
                              nsWidgetInitData *aInitData,
                              HWND hwndOwner = 0);

   
   virtual ULONG GetFCFlags();

   virtual void SubclassWindow(BOOL bState);

   PRBool  ConvertStatus( nsEventStatus aStatus)
                        { return aStatus == nsEventStatus_eConsumeNoDefault; }
   void    InitEvent( nsGUIEvent &event, nsPoint *pt = 0);
   virtual PRBool DispatchWindowEvent(nsGUIEvent* event);
   virtual PRBool DispatchWindowEvent(nsGUIEvent*event, nsEventStatus &aStatus);
   PRBool  DispatchStandardEvent( PRUint32 aMsg);
   PRBool  DispatchCommandEvent(PRUint32 aEventCommand);
   PRBool  DispatchDragDropEvent( PRUint32 aMsg);
   virtual PRBool DispatchMouseEvent(PRUint32 aEventType, MPARAM mp1, MPARAM mp2, 
                                     PRBool aIsContextMenuKey = PR_FALSE,
                                     PRInt16 aButton = nsMouseEvent::eLeftButton);
   virtual PRBool DispatchResizeEvent( PRInt32 aClientX, PRInt32 aClientY);
   void GetNonClientBounds(nsRect &aRect);
   void    DeferPosition( HWND, HWND, long, long, long, long, ULONG);
   void ConstrainZLevel(HWND *aAfter);

   PRBool   CheckDragStatus(PRUint32 aAction, HPS * oHps);
   PRBool   ReleaseIfDragHPS(HPS aHps);

   HBITMAP DataToBitmap(PRUint8* aImageData, PRUint32 aWidth,
                        PRUint32 aHeight, PRUint32 aDepth);
   HBITMAP CreateBitmapRGB(PRUint8* aImageData, PRUint32 aWidth, PRUint32 aHeight);
   
   HBITMAP CreateTransparencyMask(PRInt32  format, PRUint8* aImageData,
                                  PRUint32 aWidth, PRUint32 aHeight);

   
   enum {
      CREATE,
      DESTROY,
      SET_FOCUS,
      UPDATE_WINDOW,
      SET_TITLE,
      GET_TITLE
   };
   friend MRESULT EXPENTRY fnwpNSWindow( HWND, ULONG, MPARAM, MPARAM);
   friend MRESULT EXPENTRY fnwpFrame( HWND, ULONG, MPARAM, MPARAM);
#ifdef DEBUG_FOCUS
   int mWindowIdentifier;
#endif
};

#define PM2NS_PARENT NS2PM_PARENT
#define PM2NS NS2PM

#define PMSCAN_PADMULT      0x37
#define PMSCAN_PAD7         0x47
#define PMSCAN_PAD8         0x48
#define PMSCAN_PAD9         0x49
#define PMSCAN_PADMINUS     0x4A
#define PMSCAN_PAD4         0x4B
#define PMSCAN_PAD5         0x4C
#define PMSCAN_PAD6         0x4D
#define PMSCAN_PADPLUS      0x4E
#define PMSCAN_PAD1         0x4F
#define PMSCAN_PAD2         0x50
#define PMSCAN_PAD3         0x51
#define PMSCAN_PAD0         0x52
#define PMSCAN_PADPERIOD    0x53
#define PMSCAN_PADDIV       0x5c

#define isNumPadScanCode(scanCode) !( (scanCode < PMSCAN_PAD7) ||      \
                                      (scanCode > PMSCAN_PADPERIOD) || \
                                      (scanCode == PMSCAN_PADMULT) ||  \
                                      (scanCode == PMSCAN_PADDIV) ||   \
                                      (scanCode == PMSCAN_PADMINUS) || \
                                      (scanCode == PMSCAN_PADPLUS) )
#define isNumlockOn (BOOL)WinGetKeyState(HWND_DESKTOP, VK_NUMLOCK) & 0x0001

extern PRUint32 WMChar2KeyCode( MPARAM mp1, MPARAM mp2);

extern nsWindow *NS_HWNDToWindow( HWND hwnd);

#define NSCANVASCLASS "WarpzillaCanvas"

#if 0














#ifndef _nscanvas_h
#include "nsCanvas.h"
typedef nsCanvas ChildWindow;
#endif

#endif

#endif
