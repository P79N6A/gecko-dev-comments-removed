






































#ifndef nsCocoaWindow_h_
#define nsCocoaWindow_h_

#undef DARWIN

#import <Cocoa/Cocoa.h>

#include "nsBaseWidget.h"
#include "nsPIWidgetCocoa.h"
#include "nsAutoPtr.h"

class nsCocoaWindow;
class nsChildView;
class nsMenuBarX;

typedef struct _nsCocoaWindowList {
  _nsCocoaWindowList() : prev(NULL), window(NULL) {}
  struct _nsCocoaWindowList *prev;
  nsCocoaWindow *window; 
} nsCocoaWindowList;


@interface NSWindow (Undocumented)






- (void)_setWindowNumber:(int)aNumber;




- (void)setBottomCornerRounded:(BOOL)rounded;

@end


@interface PopupWindow : NSWindow
{
@private
  BOOL mIsContextMenu;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(unsigned int)styleMask
      backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;
- (BOOL)isContextMenu;
- (void)setIsContextMenu:(BOOL)flag;

@end


@interface BorderlessWindow : NSWindow
{
}

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;

@end


@interface WindowDelegate : NSObject
{
  nsCocoaWindow* mGeckoWindow; 
  
  
  
  PRBool mToplevelActiveState;
}
+ (void)paintMenubarForWindow:(NSWindow*)aWindow;
- (id)initWithGeckoWindow:(nsCocoaWindow*)geckoWind;
- (void)windowDidResize:(NSNotification*)aNotification;
- (void)sendFocusEvent:(PRUint32)eventType;
- (nsCocoaWindow*)geckoWidget;
- (PRBool)toplevelActiveState;
- (void)sendToplevelActivateEvents;
- (void)sendToplevelDeactivateEvents;
@end

struct UnifiedGradientInfo {
  float titlebarHeight;
  float toolbarHeight;
  BOOL windowIsMain;
  BOOL drawTitlebar; 
};



@interface TitlebarAndBackgroundColor : NSColor
{
  NSColor *mActiveTitlebarColor;
  NSColor *mInactiveTitlebarColor;
  NSColor *mBackgroundColor;
  NSWindow *mWindow; 
}

- (id)initWithActiveTitlebarColor:(NSColor*)aActiveTitlebarColor
            inactiveTitlebarColor:(NSColor*)aInactiveTitlebarColor
                  backgroundColor:(NSColor*)aBackgroundColor
                        forWindow:(NSWindow*)aWindow;


- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (NSColor*)activeTitlebarColor;
- (NSColor*)inactiveTitlebarColor;

- (void)setBackgroundColor:(NSColor*)aColor;
- (NSColor*)backgroundColor;

- (NSWindow*)window;
@end


@interface ToolbarWindow : NSWindow
{
  TitlebarAndBackgroundColor *mColor;
  float mUnifiedToolbarHeight;
  BOOL mSuppressPainting;
}
- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (void)setUnifiedToolbarHeight:(float)aToolbarHeight;
- (float)unifiedToolbarHeight;
- (float)titlebarHeight;
- (BOOL)isPaintingSuppressed;


- (NSColor*)windowBackgroundColor;
@end

class nsCocoaWindow : public nsBaseWidget, public nsPIWidgetCocoa
{
private:
  
  typedef nsBaseWidget Inherited;

public:

    nsCocoaWindow();
    virtual ~nsCocoaWindow();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSPIWIDGETCOCOA
      
    NS_IMETHOD              Create(nsNativeWidget aParent,
                                   const nsRect &aRect,
                                   EVENT_CALLBACK aHandleEventFunction,
                                   nsIDeviceContext *aContext,
                                   nsIAppShell *aAppShell = nsnull,
                                   nsIToolkit *aToolkit = nsnull,
                                   nsWidgetInitData *aInitData = nsnull);

    NS_IMETHOD              Create(nsIWidget* aParent,
                                   const nsRect &aRect,
                                   EVENT_CALLBACK aHandleEventFunction,
                                   nsIDeviceContext *aContext,
                                   nsIAppShell *aAppShell = nsnull,
                                   nsIToolkit *aToolkit = nsnull,
                                   nsWidgetInitData *aInitData = nsnull);

    NS_IMETHOD              Destroy();
     
     

    virtual nsresult        StandardCreate(nsIWidget *aParent,
                                    const nsRect &aRect,
                                    EVENT_CALLBACK aHandleEventFunction,
                                    nsIDeviceContext *aContext,
                                    nsIAppShell *aAppShell,
                                    nsIToolkit *aToolkit,
                                    nsWidgetInitData *aInitData,
                                    nsNativeWidget aNativeWindow = nsnull);

    NS_IMETHOD              Show(PRBool aState);
    virtual nsIWidget*      GetSheetWindowParent(void);
    NS_IMETHOD              AddEventListener(nsIEventListener * aListener);
    NS_IMETHOD              Enable(PRBool aState);
    NS_IMETHOD              IsEnabled(PRBool *aState);
    NS_IMETHOD              SetModal(PRBool aState);
    NS_IMETHOD              IsVisible(PRBool & aState);
    NS_IMETHOD              SetFocus(PRBool aState=PR_FALSE);
    NS_IMETHOD              SetMenuBar(void* aMenuBar);
    virtual nsMenuBarX*     GetMenuBar();
    NS_IMETHOD              ShowMenuBar(PRBool aShow);
    NS_IMETHOD WidgetToScreen(const nsRect& aOldRect, nsRect& aNewRect);
    NS_IMETHOD ScreenToWidget(const nsRect& aOldRect, nsRect& aNewRect);
    
    virtual void* GetNativeData(PRUint32 aDataType) ;

    NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
                                              PRInt32 *aX, PRInt32 *aY);
    NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
    NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, PRBool aActivate);
    NS_IMETHOD              SetSizeMode(PRInt32 aMode);

    NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
    NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
    NS_IMETHOD              GetScreenBounds(nsRect &aRect);
    virtual PRBool          OnPaint(nsPaintEvent &event);
    void                    ReportSizeEvent(NSRect *overrideRect = nsnull);

    NS_IMETHOD              SetTitle(const nsAString& aTitle);

    NS_IMETHOD Invalidate(const nsRect & aRect, PRBool aIsSynchronous);
    NS_IMETHOD Invalidate(PRBool aIsSynchronous);
    NS_IMETHOD Update();
    NS_IMETHOD Scroll(PRInt32 aDx, PRInt32 aDy, nsRect *alCipRect) { return NS_OK; }
    NS_IMETHOD SetColorMap(nsColorMap *aColorMap) { return NS_OK; }
    NS_IMETHOD BeginResizingChildren(void) { return NS_OK; }
    NS_IMETHOD EndResizingChildren(void) { return NS_OK; }
    NS_IMETHOD GetPreferredSize(PRInt32& aWidth, PRInt32& aHeight) { return NS_OK; }
    NS_IMETHOD SetPreferredSize(PRInt32 aWidth, PRInt32 aHeight) { return NS_OK; }
    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus) ;
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
    NS_IMETHOD GetAttention(PRInt32 aCycleCount);
    virtual nsTransparencyMode GetTransparencyMode();
    virtual void SetTransparencyMode(nsTransparencyMode aMode);
    NS_IMETHOD SetWindowShadowStyle(PRInt32 aStyle);
    NS_IMETHOD SetWindowTitlebarColor(nscolor aColor, PRBool aActive);

    
    void DispatchSizeModeEvent(nsSizeMode aSizeMode);

    virtual gfxASurface* GetThebesSurface();

    
    virtual PRBool DragEvent(unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers);

    
    PRBool IsResizing () const { return mIsResizing; }
    void StartResizing () { mIsResizing = PR_TRUE; }
    void StopResizing () { mIsResizing = PR_FALSE; }

    PRBool HasModalDescendents() { return mNumModalDescendents > 0; }
    NSWindow *GetCocoaWindow() { return mWindow; }

    
    NS_IMETHOD ResetInputState();
    
    void MakeBackgroundTransparent(PRBool aTransparent);

    NS_IMETHOD BeginSecureKeyboardInput();
    NS_IMETHOD EndSecureKeyboardInput();

    static void UnifiedShading(void* aInfo, const float* aIn, float* aOut);

protected:
  
  nsIWidget*           mParent;         
  NSWindow*            mWindow;         
  WindowDelegate*      mDelegate;       
  nsRefPtr<nsMenuBarX> mMenuBar;
  NSWindow*            mSheetWindowParent; 
  nsChildView*         mPopupContentView; 

  PRPackedBool         mIsResizing;     
  PRPackedBool         mWindowMadeHere; 
  PRPackedBool         mSheetNeedsShow; 
                                        
  PRPackedBool         mModal;

  PRInt32              mNumModalDescendents;
};


#endif 
