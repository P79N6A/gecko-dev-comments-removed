




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

typedef struct NSEdgeInsets {
    CGFloat top;
    CGFloat left;
    CGFloat bottom;
    CGFloat right;
} NSEdgeInsets;

#endif

typedef struct _nsCocoaWindowList {
  _nsCocoaWindowList() : prev(nullptr), window(nullptr) {}
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

  
  BOOL mDisabledNeedsDisplay;

  
  
  
  float mDPI;

  NSTrackingArea* mTrackingArea;

  NSRect mDirtyRect;

  BOOL mBeingShown;
  BOOL mDrawTitle;
  BOOL mBrightTitlebarForeground;
  BOOL mUseMenuStyle;
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

- (void)setBeingShown:(BOOL)aValue;
- (BOOL)isBeingShown;
- (BOOL)isVisibleOrBeingShown;

- (ChildView*)mainChildView;

- (NSArray*)titlebarControls;

- (void)setWantsTitleDrawn:(BOOL)aDrawTitle;
- (BOOL)wantsTitleDrawn;

- (void)setUseBrightTitlebarForeground:(BOOL)aBrightForeground;
- (BOOL)useBrightTitlebarForeground;

- (void)disableSetNeedsDisplay;
- (void)enableSetNeedsDisplay;

- (NSRect)getAndResetNativeDirtyRect;

- (void)setUseMenuStyle:(BOOL)aValue;

@end

@interface NSWindow (Undocumented)






- (void)_setWindowNumber:(NSInteger)aNumber;





- (void)setBottomCornerRounded:(BOOL)rounded;
- (BOOL)bottomCornerRounded;


- (NSRect)contentRectForFrameRect:(NSRect)windowFrame styleMask:(NSUInteger)windowStyle;
- (NSRect)frameRectForContentRect:(NSRect)windowContentRect styleMask:(NSUInteger)windowStyle;




+ (Class)frameViewClassForStyleMask:(NSUInteger)styleMask;

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
  CGFloat mUnifiedToolbarHeight;
  NSColor *mBackgroundColor;
  NSView *mTitlebarView; 
  NSRect mWindowButtonsRect;
  NSRect mFullScreenButtonRect;
}

- (void)setTitlebarColor:(NSColor*)aColor forActiveWindow:(BOOL)aActive;
- (void)setUnifiedToolbarHeight:(CGFloat)aHeight;
- (CGFloat)unifiedToolbarHeight;
- (CGFloat)titlebarHeight;
- (NSRect)titlebarRect;
- (void)setTitlebarNeedsDisplayInRect:(NSRect)aRect sync:(BOOL)aSync;
- (void)setTitlebarNeedsDisplayInRect:(NSRect)aRect;
- (void)setDrawsContentsIntoWindowFrame:(BOOL)aState;
- (void)placeWindowButtons:(NSRect)aRect;
- (void)placeFullScreenButton:(NSRect)aRect;
- (NSPoint)windowButtonsPositionWithDefaultPosition:(NSPoint)aDefaultPosition;
- (NSPoint)fullScreenButtonPositionWithDefaultPosition:(NSPoint)aDefaultPosition;
@end

class nsCocoaWindow : public nsBaseWidget, public nsPIWidgetCocoa
{
private:
  
  typedef nsBaseWidget Inherited;

public:

    nsCocoaWindow();

    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSPIWIDGETCOCOA
      
    NS_IMETHOD              Create(nsIWidget* aParent,
                                   nsNativeWidget aNativeParent,
                                   const nsIntRect &aRect,
                                   nsWidgetInitData *aInitData = nullptr) MOZ_OVERRIDE;

    NS_IMETHOD              Destroy() MOZ_OVERRIDE;

    NS_IMETHOD              Show(bool aState) MOZ_OVERRIDE;
    virtual nsIWidget*      GetSheetWindowParent(void) MOZ_OVERRIDE;
    NS_IMETHOD              Enable(bool aState) MOZ_OVERRIDE;
    virtual bool            IsEnabled() const MOZ_OVERRIDE;
    NS_IMETHOD              SetModal(bool aState) MOZ_OVERRIDE;
    virtual bool            IsVisible() const MOZ_OVERRIDE;
    NS_IMETHOD              SetFocus(bool aState=false) MOZ_OVERRIDE;
    virtual mozilla::LayoutDeviceIntPoint WidgetToScreenOffset() MOZ_OVERRIDE;
    virtual nsIntPoint GetClientOffset() MOZ_OVERRIDE;
    virtual nsIntSize ClientToWindowSize(const nsIntSize& aClientSize) MOZ_OVERRIDE;

    virtual void* GetNativeData(uint32_t aDataType) MOZ_OVERRIDE;

    NS_IMETHOD              ConstrainPosition(bool aAllowSlop,
                                              int32_t *aX, int32_t *aY) MOZ_OVERRIDE;
    virtual void            SetSizeConstraints(const SizeConstraints& aConstraints) MOZ_OVERRIDE;
    NS_IMETHOD              Move(double aX, double aY) MOZ_OVERRIDE;
    NS_IMETHOD              PlaceBehind(nsTopLevelWidgetZPlacement aPlacement,
                                        nsIWidget *aWidget, bool aActivate) MOZ_OVERRIDE;
    NS_IMETHOD              SetSizeMode(int32_t aMode) MOZ_OVERRIDE;
    NS_IMETHOD              HideWindowChrome(bool aShouldHide) MOZ_OVERRIDE;
    void                    EnteredFullScreen(bool aFullScreen);
    NS_IMETHOD              MakeFullScreen(bool aFullScreen, nsIScreen* aTargetScreen = nullptr) MOZ_OVERRIDE;
    NS_IMETHOD              Resize(double aWidth, double aHeight, bool aRepaint) MOZ_OVERRIDE;
    NS_IMETHOD              Resize(double aX, double aY, double aWidth, double aHeight, bool aRepaint) MOZ_OVERRIDE;
    NS_IMETHOD              GetClientBounds(nsIntRect &aRect) MOZ_OVERRIDE;
    NS_IMETHOD              GetScreenBounds(nsIntRect &aRect) MOZ_OVERRIDE;
    void                    ReportMoveEvent();
    void                    ReportSizeEvent();
    NS_IMETHOD              SetCursor(nsCursor aCursor) MOZ_OVERRIDE;
    NS_IMETHOD              SetCursor(imgIContainer* aCursor, uint32_t aHotspotX, uint32_t aHotspotY) MOZ_OVERRIDE;

    CGFloat                 BackingScaleFactor();
    void                    BackingScaleFactorChanged();
    virtual double          GetDefaultScaleInternal() MOZ_OVERRIDE;
    virtual int32_t         RoundsWidgetCoordinatesTo() MOZ_OVERRIDE;

    NS_IMETHOD              SetTitle(const nsAString& aTitle) MOZ_OVERRIDE;

    NS_IMETHOD Invalidate(const nsIntRect &aRect) MOZ_OVERRIDE;
    virtual nsresult ConfigureChildren(const nsTArray<Configuration>& aConfigurations) MOZ_OVERRIDE;
    virtual LayerManager* GetLayerManager(PLayerTransactionChild* aShadowManager = nullptr,
                                          LayersBackend aBackendHint = mozilla::layers::LayersBackend::LAYERS_NONE,
                                          LayerManagerPersistence aPersistence = LAYER_MANAGER_CURRENT,
                                          bool* aAllowRetaining = nullptr) MOZ_OVERRIDE;
    NS_IMETHOD DispatchEvent(mozilla::WidgetGUIEvent* aEvent,
                             nsEventStatus& aStatus) MOZ_OVERRIDE;
    NS_IMETHOD CaptureRollupEvents(nsIRollupListener * aListener, bool aDoCapture) MOZ_OVERRIDE;
    NS_IMETHOD GetAttention(int32_t aCycleCount) MOZ_OVERRIDE;
    virtual bool HasPendingInputEvent() MOZ_OVERRIDE;
    virtual nsTransparencyMode GetTransparencyMode() MOZ_OVERRIDE;
    virtual void SetTransparencyMode(nsTransparencyMode aMode) MOZ_OVERRIDE;
    NS_IMETHOD SetWindowShadowStyle(int32_t aStyle) MOZ_OVERRIDE;
    virtual void SetShowsToolbarButton(bool aShow) MOZ_OVERRIDE;
    virtual void SetShowsFullScreenButton(bool aShow) MOZ_OVERRIDE;
    virtual void SetWindowAnimationType(WindowAnimationType aType) MOZ_OVERRIDE;
    virtual void SetDrawsTitle(bool aDrawTitle) MOZ_OVERRIDE;
    virtual void SetUseBrightTitlebarForeground(bool aBrightForeground) MOZ_OVERRIDE;
    NS_IMETHOD SetNonClientMargins(nsIntMargin &margins) MOZ_OVERRIDE;
    NS_IMETHOD SetWindowTitlebarColor(nscolor aColor, bool aActive) MOZ_OVERRIDE;
    virtual void SetDrawsInTitlebar(bool aState) MOZ_OVERRIDE;
    virtual void UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries) MOZ_OVERRIDE;
    virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                                uint32_t aNativeMessage,
                                                uint32_t aModifierFlags) MOZ_OVERRIDE;

    void DispatchSizeModeEvent();

    
    virtual bool DragEvent(unsigned int aMessage, Point aMouseGlobal, UInt16 aKeyModifiers);

    bool HasModalDescendents() { return mNumModalDescendents > 0; }
    NSWindow *GetCocoaWindow() { return mWindow; }

    void SetMenuBar(nsMenuBarX* aMenuBar);
    nsMenuBarX *GetMenuBar();

    NS_IMETHOD_(void) SetInputContext(
                        const InputContext& aContext,
                        const InputContextAction& aAction) MOZ_OVERRIDE;
    NS_IMETHOD_(InputContext) GetInputContext() MOZ_OVERRIDE
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
    NS_IMETHOD_(bool) ExecuteNativeKeyBinding(
                        NativeKeyBindingsType aType,
                        const mozilla::WidgetKeyboardEvent& aEvent,
                        DoCommandCallback aCallback,
                        void* aCallbackData) MOZ_OVERRIDE;

    void SetPopupWindowLevel();

    NS_IMETHOD         ReparentNativeWidget(nsIWidget* aNewParent) MOZ_OVERRIDE;
protected:
  virtual ~nsCocoaWindow();

  nsresult             CreateNativeWindow(const NSRect &aRect,
                                          nsBorderStyle aBorderStyle,
                                          bool aRectIsFrameRect);
  nsresult             CreatePopupContentView(const nsIntRect &aRect);
  void                 DestroyNativeWindow();
  void                 AdjustWindowShadow();
  void                 SetWindowBackgroundBlur();
  void                 UpdateBounds();

  nsresult             DoResize(double aX, double aY, double aWidth, double aHeight,
                                bool aRepaint, bool aConstrainToCurrentScreen);

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget() MOZ_OVERRIDE
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_POPUP_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  virtual nsresult NotifyIMEInternal(
                     const IMENotification& aIMENotification) MOZ_OVERRIDE;

  nsIWidget*           mParent;         
  BaseWindow*          mWindow;         
  WindowDelegate*      mDelegate;       
  nsRefPtr<nsMenuBarX> mMenuBar;
  NSWindow*            mSheetWindowParent; 
  nsChildView*         mPopupContentView; 
  int32_t              mShadowStyle;

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
  bool                 mInResize; 

  int32_t              mNumModalDescendents;
  InputContext         mInputContext;
};

#endif 
