






































#ifndef nsCocoaWindow_h_
#define nsCocoaWindow_h_

#undef DARWIN

#import <Cocoa/Cocoa.h>

#include "nsBaseWidget.h"
#include "nsPIWidgetCocoa.h"
#include "nsAutoPtr.h"
#include "nsCocoaUtils.h"

class nsCocoaWindow;
class nsChildView;
class nsMenuBarX;

typedef struct _nsCocoaWindowList {
  _nsCocoaWindowList() : prev(NULL), window(NULL) {}
  struct _nsCocoaWindowList *prev;
  nsCocoaWindow *window; 
} nsCocoaWindowList;








@interface BaseWindow : NSWindow
{
  
  NSMutableDictionary* mState;
  BOOL mDrawsIntoWindowFrame;
  NSColor* mActiveTitlebarColor;
  NSColor* mInactiveTitlebarColor;

  
  BOOL mScheduledShadowInvalidation;
}

- (void)importState:(NSDictionary*)aState;
- (NSMutableDictionary*)exportState;
- (void)setDrawsContentsIntoWindowFrame:(BOOL)aState;
- (BOOL)drawsContentsIntoWindowFrame;
- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (NSColor*)titlebarColorForActiveWindow:(BOOL)aActive;

- (void)deferredInvalidateShadow;
- (void)invalidateShadow;

@end

@interface NSWindow (Undocumented)






- (void)_setWindowNumber:(NSInteger)aNumber;




- (void)setBottomCornerRounded:(BOOL)rounded;

@end

@interface PopupWindow : BaseWindow
{
@private
  BOOL mIsContextMenu;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)styleMask
      backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation;
- (BOOL)isContextMenu;
- (void)setIsContextMenu:(BOOL)flag;

@end

@interface BorderlessWindow : BaseWindow
{
}

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;

@end

@interface WindowDelegate : NSObject
{
  nsCocoaWindow* mGeckoWindow; 
  
  
  
  PRBool mToplevelActiveState;
  BOOL mHasEverBeenZoomed;
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

@class ToolbarWindow;



@interface TitlebarAndBackgroundColor : NSColor
{
  ToolbarWindow *mWindow; 
}

- (id)initWithWindow:(ToolbarWindow*)aWindow;

@end


@interface ToolbarWindow : BaseWindow
{
  TitlebarAndBackgroundColor *mColor;
  float mUnifiedToolbarHeight;
  NSColor *mBackgroundColor;
}

- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (void)setUnifiedToolbarHeight:(float)aToolbarHeight;
- (float)unifiedToolbarHeight;
- (float)titlebarHeight;
- (NSRect)titlebarRect;
- (void)setTitlebarNeedsDisplayInRect:(NSRect)aRect sync:(BOOL)aSync;
- (void)setTitlebarNeedsDisplayInRect:(NSRect)aRect;
- (void)setDrawsContentsIntoWindowFrame:(BOOL)aState;
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
      
    NS_IMETHOD              Create(nsIWidget* aParent,
                                   nsNativeWidget aNativeParent,
                                   const nsIntRect &aRect,
                                   EVENT_CALLBACK aHandleEventFunction,
                                   nsIDeviceContext *aContext,
                                   nsIAppShell *aAppShell = nsnull,
                                   nsIToolkit *aToolkit = nsnull,
                                   nsWidgetInitData *aInitData = nsnull);

    NS_IMETHOD              Destroy();

    NS_IMETHOD              Show(PRBool aState);
    virtual nsIWidget*      GetSheetWindowParent(void);
    NS_IMETHOD              Enable(PRBool aState);
    NS_IMETHOD              IsEnabled(PRBool *aState);
    NS_IMETHOD              SetModal(PRBool aState);
    NS_IMETHOD              IsVisible(PRBool & aState);
    NS_IMETHOD              SetFocus(PRBool aState=PR_FALSE);
    virtual nsIntPoint WidgetToScreenOffset();
    
    virtual void* GetNativeData(PRUint32 aDataType) ;

    NS_IMETHOD              ConstrainPosition(PRBool aAllowSlop,
                                              PRInt32 *aX, PRInt32 *aY);
    NS_IMETHOD              Move(PRInt32 aX, PRInt32 aY);
    NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, PRBool aActivate);
    NS_IMETHOD              SetSizeMode(PRInt32 aMode);
    NS_IMETHOD              HideWindowChrome(PRBool aShouldHide);
    NS_IMETHOD              MakeFullScreen(PRBool aFullScreen);
    NS_IMETHOD              Resize(PRInt32 aWidth,PRInt32 aHeight, PRBool aRepaint);
    NS_IMETHOD              Resize(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight, PRBool aRepaint);
    NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
    void                    ReportSizeEvent(NSRect *overrideRect = nsnull);
    NS_IMETHOD              SetCursor(nsCursor aCursor);
    NS_IMETHOD              SetCursor(imgIContainer* aCursor, PRUint32 aHotspotX, PRUint32 aHotspotY);

    NS_IMETHOD              SetTitle(const nsAString& aTitle);

    NS_IMETHOD Invalidate(const nsIntRect &aRect, PRBool aIsSynchronous);
    NS_IMETHOD Update();
    virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
    virtual void Scroll(const nsIntPoint& aDelta,
                        const nsTArray<nsIntRect>& aDestRects,
                        const nsTArray<Configuration>& aConfigurations);
    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus) ;
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, PRBool aDoCapture, PRBool aConsumeRollupEvent);
    NS_IMETHOD GetAttention(PRInt32 aCycleCount);
    virtual PRBool HasPendingInputEvent();
    virtual nsTransparencyMode GetTransparencyMode();
    virtual void SetTransparencyMode(nsTransparencyMode aMode);
    NS_IMETHOD SetWindowShadowStyle(PRInt32 aStyle);
    virtual void SetShowsToolbarButton(PRBool aShow);
    NS_IMETHOD SetWindowTitlebarColor(nscolor aColor, PRBool aActive);
    virtual void SetDrawsInTitlebar(PRBool aState);
    virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                PRUint32 aNativeMessage,
                                                PRUint32 aModifierFlags);

    void DispatchSizeModeEvent();

    virtual gfxASurface* GetThebesSurface();

    
    virtual PRBool DragEvent(unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers);

    
    PRBool IsResizing () const { return mIsResizing; }
    void StartResizing () { mIsResizing = PR_TRUE; }
    void StopResizing () { mIsResizing = PR_FALSE; }

    PRBool HasModalDescendents() { return mNumModalDescendents > 0; }
    NSWindow *GetCocoaWindow() { return mWindow; }

    void SetMenuBar(nsMenuBarX* aMenuBar);
    nsMenuBarX *GetMenuBar();

    
    NS_IMETHOD ResetInputState();
    
    NS_IMETHOD BeginSecureKeyboardInput();
    NS_IMETHOD EndSecureKeyboardInput();

    static void UnifiedShading(void* aInfo, const CGFloat* aIn, CGFloat* aOut);

protected:

  nsresult             CreateNativeWindow(const NSRect &aRect,
                                          nsBorderStyle aBorderStyle,
                                          PRBool aRectIsFrameRect);
  nsresult             CreatePopupContentView(const nsIntRect &aRect,
                                              EVENT_CALLBACK aHandleEventFunction,
                                              nsIDeviceContext *aContext,
                                              nsIAppShell *aAppShell,
                                              nsIToolkit *aToolkit);
  void                 DestroyNativeWindow();

  nsIWidget*           mParent;         
  BaseWindow*          mWindow;         
  WindowDelegate*      mDelegate;       
  nsRefPtr<nsMenuBarX> mMenuBar;
  NSWindow*            mSheetWindowParent; 
  nsChildView*         mPopupContentView; 

  PRPackedBool         mIsResizing;     
  PRPackedBool         mWindowMadeHere; 
  PRPackedBool         mSheetNeedsShow; 
                                        
  PRPackedBool         mFullScreen;
  PRPackedBool         mModal;

  PRInt32              mNumModalDescendents;
};

#endif 
