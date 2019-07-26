




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
@class ChildView;


#define CUIDRAW_MAX_AREA 500000



#if !defined(MAC_OS_X_VERSION_10_7) || \
    MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7

enum {
    NSWindowAnimationBehaviorDefault = 0,
    NSWindowAnimationBehaviorNone = 2,
    NSWindowAnimationBehaviorDocumentWindow = 3,
    NSWindowAnimationBehaviorUtilityWindow = 4,
    NSWindowAnimationBehaviorAlertPanel = 5,
    NSWindowCollectionBehaviorFullScreenPrimary = 128, 
};

typedef NSInteger NSWindowAnimationBehavior;

@interface NSWindow (LionWindowFeatures)
- (void)setAnimationBehavior:(NSWindowAnimationBehavior)newAnimationBehavior;
- (void)toggleFullScreen:(id)sender;
@end

#endif

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

  
  
  
  float mDPI;

  NSTrackingArea* mTrackingArea;
}

- (void)importState:(NSDictionary*)aState;
- (NSMutableDictionary*)exportState;
- (void)setDrawsContentsIntoWindowFrame:(BOOL)aState;
- (BOOL)drawsContentsIntoWindowFrame;
- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (NSColor*)titlebarColorForActiveWindow:(BOOL)aActive;

- (void)deferredInvalidateShadow;
- (void)invalidateShadow;
- (float)getDPI;

- (void)mouseEntered:(NSEvent*)aEvent;
- (void)mouseExited:(NSEvent*)aEvent;
- (void)mouseMoved:(NSEvent*)aEvent;
- (void)updateTrackingArea;
- (NSView*)trackingAreaView;

@end

@interface NSWindow (Undocumented)






- (void)_setWindowNumber:(NSInteger)aNumber;





- (void)setBottomCornerRounded:(BOOL)rounded;
- (BOOL)bottomCornerRounded;

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
- (BOOL)canBecomeMainWindow;

@end

@interface BorderlessWindow : BaseWindow
{
}

- (BOOL)canBecomeKeyWindow;
- (BOOL)canBecomeMainWindow;

@end

#if defined( MAC_OS_X_VERSION_10_6 ) && ( MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6 )
@interface WindowDelegate : NSObject <NSWindowDelegate>
#else
@interface WindowDelegate : NSObject
#endif
{
  nsCocoaWindow* mGeckoWindow; 
  
  
  
  bool mToplevelActiveState;
  BOOL mHasEverBeenZoomed;
}
+ (void)paintMenubarForWindow:(NSWindow*)aWindow;
- (id)initWithGeckoWindow:(nsCocoaWindow*)geckoWind;
- (void)windowDidResize:(NSNotification*)aNotification;
- (nsCocoaWindow*)geckoWidget;
- (bool)toplevelActiveState;
- (void)sendToplevelActivateEvents;
- (void)sendToplevelDeactivateEvents;
@end

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
  NSView *mTitlebarView; 
}

- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (void)setUnifiedToolbarHeight:(float)aHeight;
- (float)unifiedToolbarHeight;
- (float)titlebarHeight;
- (NSRect)titlebarRect;
- (void)setTitlebarNeedsDisplayInRect:(NSRect)aRect sync:(BOOL)aSync;
- (void)setTitlebarNeedsDisplayInRect:(NSRect)aRect;
- (void)setDrawsContentsIntoWindowFrame:(BOOL)aState;
- (ChildView*)mainChildView;
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
                                   nsDeviceContext *aContext,
                                   nsWidgetInitData *aInitData = nullptr);

    NS_IMETHOD              Destroy();

    NS_IMETHOD              Show(bool aState);
    virtual nsIWidget*      GetSheetWindowParent(void);
    NS_IMETHOD              Enable(bool aState);
    virtual bool            IsEnabled() const;
    NS_IMETHOD              SetModal(bool aState);
    virtual bool            IsVisible() const;
    NS_IMETHOD              SetFocus(bool aState=false);
    virtual nsIntPoint WidgetToScreenOffset();
    virtual nsIntPoint GetClientOffset();
    virtual nsIntSize ClientToWindowSize(const nsIntSize& aClientSize);

    virtual void* GetNativeData(uint32_t aDataType) ;

    NS_IMETHOD              ConstrainPosition(bool aAllowSlop,
                                              int32_t *aX, int32_t *aY);
    virtual void            SetSizeConstraints(const SizeConstraints& aConstraints);
    NS_IMETHOD              Move(double aX, double aY);
    NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, bool aActivate);
    NS_IMETHOD              SetSizeMode(int32_t aMode);
    NS_IMETHOD              HideWindowChrome(bool aShouldHide);
    void                    EnteredFullScreen(bool aFullScreen);
    NS_IMETHOD              MakeFullScreen(bool aFullScreen);
    NS_IMETHOD              Resize(double aWidth, double aHeight, bool aRepaint);
    NS_IMETHOD              Resize(double aX, double aY, double aWidth, double aHeight, bool aRepaint);
    NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
    NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);
    void                    ReportMoveEvent();
    void                    ReportSizeEvent();
    NS_IMETHOD              SetCursor(nsCursor aCursor);
    NS_IMETHOD              SetCursor(imgIContainer* aCursor, uint32_t aHotspotX, uint32_t aHotspotY);

    CGFloat                 BackingScaleFactor();
    void                    BackingScaleFactorChanged();
    virtual double          GetDefaultScaleInternal();

    NS_IMETHOD              SetTitle(const nsAString& aTitle);

    NS_IMETHOD Invalidate(const nsIntRect &aRect);
    virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
    virtual LayerManager* GetLayerManager(PLayersChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr);
    NS_IMETHOD DispatchEvent(nsGUIEvent* event, nsEventStatus & aStatus) ;
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, bool aDoCapture);
    NS_IMETHOD GetAttention(int32_t aCycleCount);
    virtual bool HasPendingInputEvent();
    virtual nsTransparencyMode GetTransparencyMode();
    virtual void SetTransparencyMode(nsTransparencyMode aMode);
    NS_IMETHOD SetWindowShadowStyle(int32_t aStyle);
    virtual void SetShowsToolbarButton(bool aShow);
    virtual void SetShowsFullScreenButton(bool aShow);
    virtual void SetWindowAnimationType(WindowAnimationType aType);
    NS_IMETHOD SetWindowTitlebarColor(nscolor aColor, bool aActive);
    virtual void SetDrawsInTitlebar(bool aState);
    virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                uint32_t aNativeMessage,
                                                uint32_t aModifierFlags);

    void DispatchSizeModeEvent();

    virtual gfxASurface* GetThebesSurface();

    
    virtual bool DragEvent(unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers);

    bool HasModalDescendents() { return mNumModalDescendents > 0; }
    NSWindow *GetCocoaWindow() { return mWindow; }

    void SetMenuBar(nsMenuBarX* aMenuBar);
    nsMenuBarX *GetMenuBar();

    NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                      const InputContextAction& aAction)
    {
      mInputContext = aContext;
    }
    NS_IMETHOD_(InputContext) GetInputContext()
    {
      NSView* view = mWindow ? [mWindow contentView] : nil;
      if (view) {
        mInputContext.mNativeIMEContext = [view inputContext];
      }
      
      
      
      if (!mInputContext.mNativeIMEContext) {
        mInputContext.mNativeIMEContext = this;
      }
      return mInputContext;
    }
    NS_IMETHOD BeginSecureKeyboardInput();
    NS_IMETHOD EndSecureKeyboardInput();

    void SetPopupWindowLevel();

    bool IsChildInFailingLeftClickThrough(NSView *aChild);
    bool ShouldFocusPlugin();

    NS_IMETHOD         ReparentNativeWidget(nsIWidget* aNewParent);
protected:

  nsresult             CreateNativeWindow(const NSRect &aRect,
                                          nsBorderStyle aBorderStyle,
                                          bool aRectIsFrameRect);
  nsresult             CreatePopupContentView(const nsIntRect &aRect,
                                              nsDeviceContext *aContext);
  void                 DestroyNativeWindow();
  void                 AdjustWindowShadow();
  void                 SetUpWindowFilter();
  void                 CleanUpWindowFilter();
  void                 UpdateBounds();

  nsresult             DoResize(double aX, double aY, double aWidth, double aHeight,
                                bool aRepaint, bool aConstrainToCurrentScreen);

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_POPUP_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  nsIWidget*           mParent;         
  BaseWindow*          mWindow;         
  WindowDelegate*      mDelegate;       
  nsRefPtr<nsMenuBarX> mMenuBar;
  NSWindow*            mSheetWindowParent; 
  nsChildView*         mPopupContentView; 
  int32_t              mShadowStyle;
  NSUInteger           mWindowFilter;

  CGFloat              mBackingScaleFactor;

  WindowAnimationType  mAnimationType;

  bool                 mWindowMadeHere; 
  bool                 mSheetNeedsShow; 
                                        
  bool                 mFullScreen;
  bool                 mInFullScreenTransition; 
                                                
  bool                 mModal;

  bool                 mUsesNativeFullScreen; 

  bool                 mIsAnimationSuppressed;

  bool                 mInReportMoveEvent; 

  int32_t              mNumModalDescendents;
  InputContext         mInputContext;
};

#endif 
