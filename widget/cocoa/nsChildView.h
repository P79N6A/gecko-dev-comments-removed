




#ifndef nsChildView_h_
#define nsChildView_h_


#include "mozView.h"
#ifdef ACCESSIBILITY
#include "mozilla/a11y/Accessible.h"
#include "mozAccessibleProtocol.h"
#endif

#include "nsAutoPtr.h"
#include "nsISupports.h"
#include "nsBaseWidget.h"
#include "nsIPluginInstanceOwner.h"
#include "nsIPluginWidget.h"
#include "nsWeakPtr.h"
#include "TextInputHandler.h"
#include "nsCocoaUtils.h"
#include "gfxQuartzSurface.h"
#include "GLContextTypes.h"
#include "mozilla/Mutex.h"
#include "nsRegion.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/UniquePtr.h"

#include "nsString.h"
#include "nsIDragService.h"

#include "npapi.h"

#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#import <AppKit/NSOpenGL.h>







#ifdef __cplusplus
extern "C" {
#endif
  #if !defined(__QUICKDRAWAPI__)

  extern void SetPort(GrafPtr port)
    __attribute__((weak_import));
  extern void SetOrigin(short h, short v)
    __attribute__((weak_import));
  extern RgnHandle NewRgn(void)
    __attribute__((weak_import));
  extern void DisposeRgn(RgnHandle rgn)
    __attribute__((weak_import));
  extern void RectRgn(RgnHandle rgn, const Rect * r)
    __attribute__((weak_import));
  extern GDHandle GetMainDevice(void)
    __attribute__((weak_import));
  extern Boolean IsPortOffscreen(CGrafPtr port)
    __attribute__((weak_import));
  extern void SetPortVisibleRegion(CGrafPtr port, RgnHandle visRgn)
    __attribute__((weak_import));
  extern void SetPortClipRegion(CGrafPtr port, RgnHandle clipRgn)
    __attribute__((weak_import));
  extern CGrafPtr GetQDGlobalsThePort(void)
    __attribute__((weak_import));

  #endif

  #if !defined(__QDOFFSCREEN__)

  extern void GetGWorld(CGrafPtr *  port, GDHandle *  gdh)
    __attribute__((weak_import));
  extern void SetGWorld(CGrafPtr port, GDHandle gdh)
    __attribute__((weak_import));

  #endif
#ifdef __cplusplus
}
#endif

class gfxASurface;
class nsChildView;
class nsCocoaWindow;
union nsPluginPort;

namespace {
class GLPresenter;
class RectTextureImage;
}

namespace mozilla {
class VibrancyManager;
namespace layers {
class GLManager;
class APZCTreeManager;
}
}

@interface NSEvent (Undocumented)




- (EventRef)_eventRef;

@end

@interface NSView (Undocumented)











- (void)_drawTitleBar:(NSRect)aRect;








- (NSRect)_dirtyRect;







- (void)_tileTitlebarAndRedisplay:(BOOL)redisplay;

@end

#if !defined(MAC_OS_X_VERSION_10_6) || \
MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_6
@interface NSEvent (SnowLeopardEventFeatures)
+ (NSUInteger)pressedMouseButtons;
+ (NSUInteger)modifierFlags;
@end
#endif















#if !defined(MAC_OS_X_VERSION_10_7) || MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_7
enum {
   NSFullScreenWindowMask = 1 << 14
};

@interface NSWindow (LionWindowFeatures)
- (NSRect)convertRectToScreen:(NSRect)aRect;
@end

#ifdef __LP64__
enum {
  NSEventSwipeTrackingLockDirection = 0x1 << 0,
  NSEventSwipeTrackingClampGestureAmount = 0x1 << 1
};
typedef NSUInteger NSEventSwipeTrackingOptions;

enum {
  NSEventGestureAxisNone = 0,
  NSEventGestureAxisHorizontal,
  NSEventGestureAxisVertical
};
typedef NSInteger NSEventGestureAxis;

@interface NSEvent (FluidSwipeTracking)
+ (BOOL)isSwipeTrackingFromScrollEventsEnabled;
- (BOOL)hasPreciseScrollingDeltas;
- (CGFloat)scrollingDeltaX;
- (CGFloat)scrollingDeltaY;
- (NSEventPhase)phase;
- (void)trackSwipeEventWithOptions:(NSEventSwipeTrackingOptions)options
          dampenAmountThresholdMin:(CGFloat)minDampenThreshold
                               max:(CGFloat)maxDampenThreshold
                      usingHandler:(void (^)(CGFloat gestureAmount, NSEventPhase phase, BOOL isComplete, BOOL *stop))trackingHandler;
@end
#endif 
#endif 

@interface ChildView : NSView<
#ifdef ACCESSIBILITY
                              mozAccessible,
#endif
                              mozView, NSTextInput, NSTextInputClient>
{
@private
  
  
  nsChildView* mGeckoChild;

  
  
  
  
  
  
  
  mozilla::widget::TextInputHandler* mTextInputHandler;  

  BOOL mIsPluginView;
  NPEventModel mPluginEventModel;
  NPDrawingModel mPluginDrawingModel;

  
  NSEvent* mLastMouseDownEvent;

  
  BOOL mBlockedLastMouseDown;

  
  NSEvent* mClickThroughMouseDownEvent;

  
  NSMutableArray* mPendingDirtyRects;
  BOOL mPendingFullDisplay;
  BOOL mPendingDisplay;

  
  
  
  BOOL mExpectingWheelStop;

  
  
  
  
  
  nsIDragService* mDragService;

  NSOpenGLContext *mGLContext;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  enum {
    eGestureState_None,
    eGestureState_StartGesture,
    eGestureState_MagnifyGesture,
    eGestureState_RotateGesture
  } mGestureState;
  float mCumulativeMagnification;
  float mCumulativeRotation;

  BOOL mDidForceRefreshOpenGL;
  BOOL mWaitingForPaint;

#ifdef __LP64__
  
  BOOL* mCancelSwipeAnimation;
  uint32_t mCurrentSwipeDir;
#endif

  
  BOOL mUsingOMTCompositor;

  
  
  CGImageRef mTopLeftCornerMask;
}


+ (void)initialize;

+ (void)registerViewForDraggedTypes:(NSView*)aView;


- (void)viewsWindowDidBecomeKey;
- (void)viewsWindowDidResignKey;


- (void)delayedTearDown;

- (void)sendFocusEvent:(uint32_t)eventType;

- (void)handleMouseMoved:(NSEvent*)aEvent;

- (void)sendMouseEnterOrExitEvent:(NSEvent*)aEvent
                            enter:(BOOL)aEnter
                             type:(mozilla::WidgetMouseEvent::exitType)aType;

- (void)updateGLContext;
- (void)_surfaceNeedsUpdate:(NSNotification*)notification;

- (BOOL)isPluginView;



- (BOOL)isInFailingLeftClickThrough;

- (void)setGLContext:(NSOpenGLContext *)aGLContext;
- (bool)preRender:(NSOpenGLContext *)aGLContext;
- (void)postRender:(NSOpenGLContext *)aGLContext;

- (BOOL)isCoveringTitlebar;











- (void)swipeWithEvent:(NSEvent *)anEvent;
- (void)beginGestureWithEvent:(NSEvent *)anEvent;
- (void)magnifyWithEvent:(NSEvent *)anEvent;
- (void)smartMagnifyWithEvent:(NSEvent *)anEvent;
- (void)rotateWithEvent:(NSEvent *)anEvent;
- (void)endGestureWithEvent:(NSEvent *)anEvent;

- (void)scrollWheel:(NSEvent *)anEvent;
- (void)handleAsyncScrollEvent:(CGEventRef)cgEvent ofType:(CGEventType)type;


+ (BOOL)isLionSmartMagnifyEvent:(NSEvent*)anEvent;


#ifdef __LP64__
- (void)maybeTrackScrollEventAsSwipe:(NSEvent *)anEvent
                     scrollOverflowX:(double)anOverflowX
                     scrollOverflowY:(double)anOverflowY
              viewPortIsOverscrolled:(BOOL)aViewPortIsOverscrolled;
#endif

- (void)setUsingOMTCompositor:(BOOL)aUseOMTC;
@end

class ChildViewMouseTracker {

public:

  static void MouseMoved(NSEvent* aEvent);
  static void MouseScrolled(NSEvent* aEvent);
  static void OnDestroyView(ChildView* aView);
  static void OnDestroyWindow(NSWindow* aWindow);
  static BOOL WindowAcceptsEvent(NSWindow* aWindow, NSEvent* aEvent,
                                 ChildView* aView, BOOL isClickThrough = NO);
  static void MouseExitedWindow(NSEvent* aEvent);
  static void MouseEnteredWindow(NSEvent* aEvent);
  static void ReEvaluateMouseEnterState(NSEvent* aEvent = nil, ChildView* aOldView = nil);
  static void ResendLastMouseMoveEvent();
  static ChildView* ViewForEvent(NSEvent* aEvent);
  static void AttachPluginEvent(mozilla::WidgetMouseEventBase& aMouseEvent,
                                ChildView* aView,
                                NSEvent* aNativeMouseEvent,
                                int aPluginEventType,
                                void* aPluginEventHolder);

  static ChildView* sLastMouseEventView;
  static NSEvent* sLastMouseMoveEvent;
  static NSWindow* sWindowUnderMouse;
  static NSPoint sLastScrollEventScreenLocation;
};







class nsChildView : public nsBaseWidget,
                    public nsIPluginWidget
{
private:
  typedef nsBaseWidget Inherited;
  typedef mozilla::layers::APZCTreeManager APZCTreeManager;

public:
  nsChildView();

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD              Create(nsIWidget *aParent,
                                 nsNativeWidget aNativeParent,
                                 const nsIntRect &aRect,
                                 nsDeviceContext *aContext,
                                 nsWidgetInitData *aInitData = nullptr);

  NS_IMETHOD              Destroy();

  NS_IMETHOD              Show(bool aState);
  virtual bool            IsVisible() const;

  NS_IMETHOD              SetParent(nsIWidget* aNewParent);
  virtual nsIWidget*      GetParent(void);
  virtual float           GetDPI();

  NS_IMETHOD              ConstrainPosition(bool aAllowSlop,
                                            int32_t *aX, int32_t *aY);
  NS_IMETHOD              Move(double aX, double aY);
  NS_IMETHOD              Resize(double aWidth, double aHeight, bool aRepaint);
  NS_IMETHOD              Resize(double aX, double aY,
                                 double aWidth, double aHeight, bool aRepaint);

  NS_IMETHOD              Enable(bool aState);
  virtual bool            IsEnabled() const;
  NS_IMETHOD              SetFocus(bool aRaise);
  NS_IMETHOD              GetBounds(nsIntRect &aRect);
  NS_IMETHOD              GetClientBounds(nsIntRect &aRect);
  NS_IMETHOD              GetScreenBounds(nsIntRect &aRect);

  
  
  
  
  
  
  CGFloat                 BackingScaleFactor() const;

  
  
  void                    BackingScaleFactorChanged();

  virtual double          GetDefaultScaleInternal();

  virtual int32_t         RoundsWidgetCoordinatesTo() MOZ_OVERRIDE;

  NS_IMETHOD              Invalidate(const nsIntRect &aRect);

  virtual void*           GetNativeData(uint32_t aDataType);
  virtual nsresult        ConfigureChildren(const nsTArray<Configuration>& aConfigurations);
  virtual nsIntPoint      WidgetToScreenOffset();
  virtual bool            ShowsResizeIndicator(nsIntRect* aResizerRect);

  static  bool            ConvertStatus(nsEventStatus aStatus)
                          { return aStatus == nsEventStatus_eConsumeNoDefault; }
  NS_IMETHOD              DispatchEvent(mozilla::WidgetGUIEvent* aEvent,
                                        nsEventStatus& aStatus);

  virtual bool            ComputeShouldAccelerate(bool aDefault);
  virtual bool            ShouldUseOffMainThreadCompositing() MOZ_OVERRIDE;

  NS_IMETHOD        SetCursor(nsCursor aCursor);
  NS_IMETHOD        SetCursor(imgIContainer* aCursor, uint32_t aHotspotX, uint32_t aHotspotY);

  NS_IMETHOD        CaptureRollupEvents(nsIRollupListener * aListener, bool aDoCapture);
  NS_IMETHOD        SetTitle(const nsAString& title);

  NS_IMETHOD        GetAttention(int32_t aCycleCount);

  virtual bool HasPendingInputEvent();

  NS_IMETHOD        ActivateNativeMenuItemAt(const nsAString& indexString);
  NS_IMETHOD        ForceUpdateNativeMenuAt(const nsAString& indexString);

  NS_IMETHOD        NotifyIME(const IMENotification& aIMENotification) MOZ_OVERRIDE;
  NS_IMETHOD_(void) SetInputContext(const InputContext& aContext,
                                    const InputContextAction& aAction);
  NS_IMETHOD_(InputContext) GetInputContext();
  NS_IMETHOD        AttachNativeKeyEvent(mozilla::WidgetKeyboardEvent& aEvent);
  NS_IMETHOD_(bool) ExecuteNativeKeyBinding(
                      NativeKeyBindingsType aType,
                      const mozilla::WidgetKeyboardEvent& aEvent,
                      DoCommandCallback aCallback,
                      void* aCallbackData) MOZ_OVERRIDE;
  virtual nsIMEUpdatePreference GetIMEUpdatePreference() MOZ_OVERRIDE;
  NS_IMETHOD        GetToggledKeyState(uint32_t aKeyCode,
                                       bool* aLEDState);

  
  
  NS_IMETHOD        GetPluginClipRect(nsIntRect& outClipRect, nsIntPoint& outOrigin, bool& outWidgetVisible);
  NS_IMETHOD        StartDrawPlugin();
  NS_IMETHOD        EndDrawPlugin();
  NS_IMETHOD        SetPluginInstanceOwner(nsIPluginInstanceOwner* aInstanceOwner);

  NS_IMETHOD        SetPluginEventModel(int inEventModel);
  NS_IMETHOD        GetPluginEventModel(int* outEventModel);
  NS_IMETHOD        SetPluginDrawingModel(int inDrawingModel);

  NS_IMETHOD        StartComplexTextInputForCurrentEvent();

  virtual nsTransparencyMode GetTransparencyMode();
  virtual void                SetTransparencyMode(nsTransparencyMode aMode);

  virtual nsresult SynthesizeNativeKeyEvent(int32_t aNativeKeyboardLayout,
                                            int32_t aNativeKeyCode,
                                            uint32_t aModifierFlags,
                                            const nsAString& aCharacters,
                                            const nsAString& aUnmodifiedCharacters);

  virtual nsresult SynthesizeNativeMouseEvent(nsIntPoint aPoint,
                                              uint32_t aNativeMessage,
                                              uint32_t aModifierFlags);

  virtual nsresult SynthesizeNativeMouseMove(nsIntPoint aPoint)
  { return SynthesizeNativeMouseEvent(aPoint, NSMouseMoved, 0); }

  
  
  virtual bool      DispatchWindowEvent(mozilla::WidgetGUIEvent& event);

  void WillPaintWindow();
  bool PaintWindow(nsIntRegion aRegion);

#ifdef ACCESSIBILITY
  already_AddRefed<mozilla::a11y::Accessible> GetDocumentAccessible();
#endif

  virtual CompositorParent* NewCompositorParent(int aSurfaceWidth, int aSurfaceHeight);
  virtual void CreateCompositor();
  virtual void PrepareWindowEffects() MOZ_OVERRIDE;
  virtual void CleanupWindowEffects() MOZ_OVERRIDE;
  virtual bool PreRender(LayerManagerComposite* aManager) MOZ_OVERRIDE;
  virtual void PostRender(LayerManagerComposite* aManager) MOZ_OVERRIDE;
  virtual void DrawWindowOverlay(LayerManagerComposite* aManager, nsIntRect aRect) MOZ_OVERRIDE;

  virtual void UpdateThemeGeometries(const nsTArray<ThemeGeometry>& aThemeGeometries);

  virtual void UpdateWindowDraggingRegion(const nsIntRegion& aRegion) MOZ_OVERRIDE;
  const nsIntRegion& GetDraggableRegion() { return mDraggableRegion; }

  void              HidePlugin();
  void              UpdatePluginPort();

  void              ResetParent();

  static bool DoHasPendingInputEvent();
  static uint32_t GetCurrentInputEventCount();
  static void UpdateCurrentInputEventCount();

  NSView<mozView>* GetEditorView();

  bool IsPluginView() { return (mWindowType == eWindowType_plugin); }

  nsCocoaWindow*    GetXULWindowWidget();

  NS_IMETHOD        ReparentNativeWidget(nsIWidget* aNewParent);

  mozilla::widget::TextInputHandler* GetTextInputHandler()
  {
    return mTextInputHandler;
  }

  void              NotifyDirtyRegion(const nsIntRegion& aDirtyRegion);
  void              ClearVibrantAreas();

  
  int32_t           CocoaPointsToDevPixels(CGFloat aPts) const {
    return nsCocoaUtils::CocoaPointsToDevPixels(aPts, BackingScaleFactor());
  }
  nsIntPoint        CocoaPointsToDevPixels(const NSPoint& aPt) const {
    return nsCocoaUtils::CocoaPointsToDevPixels(aPt, BackingScaleFactor());
  }
  nsIntRect         CocoaPointsToDevPixels(const NSRect& aRect) const {
    return nsCocoaUtils::CocoaPointsToDevPixels(aRect, BackingScaleFactor());
  }
  CGFloat           DevPixelsToCocoaPoints(int32_t aPixels) const {
    return nsCocoaUtils::DevPixelsToCocoaPoints(aPixels, BackingScaleFactor());
  }
  NSRect            DevPixelsToCocoaPoints(const nsIntRect& aRect) const {
    return nsCocoaUtils::DevPixelsToCocoaPoints(aRect, BackingScaleFactor());
  }

  mozilla::TemporaryRef<mozilla::gfx::DrawTarget> StartRemoteDrawing() MOZ_OVERRIDE;
  void EndRemoteDrawing() MOZ_OVERRIDE;
  void CleanupRemoteDrawing() MOZ_OVERRIDE;

  APZCTreeManager* APZCTM() { return mAPZCTreeManager; }

protected:
  virtual ~nsChildView();

  void              ReportMoveEvent();
  void              ReportSizeEvent();

  
  
  virtual NSView*   CreateCocoaView(NSRect inFrame);
  void              TearDownView();

  virtual already_AddRefed<nsIWidget>
  AllocateChildPopupWidget()
  {
    static NS_DEFINE_IID(kCPopUpCID, NS_POPUP_CID);
    nsCOMPtr<nsIWidget> widget = do_CreateInstance(kCPopUpCID);
    return widget.forget();
  }

  void DoRemoteComposition(const nsIntRect& aRenderRect);

  
  void DrawWindowOverlay(mozilla::layers::GLManager* aManager, nsIntRect aRect);
  void MaybeDrawResizeIndicator(mozilla::layers::GLManager* aManager, const nsIntRect& aRect);
  void MaybeDrawRoundedCorners(mozilla::layers::GLManager* aManager, const nsIntRect& aRect);
  void MaybeDrawTitlebar(mozilla::layers::GLManager* aManager, const nsIntRect& aRect);

  
  
  void UpdateTitlebarCGContext();

  nsIntRect RectContainingTitlebarControls();
  void UpdateVibrancy(const nsTArray<ThemeGeometry>& aThemeGeometries);
  mozilla::VibrancyManager& EnsureVibrancyManager();

  nsIWidget* GetWidgetForListenerEvents();

protected:

  NSView<mozView>*      mView;      
  nsRefPtr<mozilla::widget::TextInputHandler> mTextInputHandler;
  InputContext          mInputContext;

  NSView<mozView>*      mParentView;
  nsIWidget*            mParentWidget;

#ifdef ACCESSIBILITY
  
  
  nsWeakPtr             mAccessible;
#endif

  
  
  mozilla::Mutex mViewTearDownLock;

  mozilla::Mutex mEffectsLock;

  
  
  bool mShowsResizeIndicator;
  nsIntRect mResizeIndicatorRect;
  bool mHasRoundedBottomCorners;
  int mDevPixelCornerRadius;
  bool mIsCoveringTitlebar;
  bool mIsFullscreen;
  nsIntRect mTitlebarRect;

  
  
  nsIntRegion mUpdatedTitlebarRegion;
  CGContextRef mTitlebarCGContext;

  
  nsAutoPtr<RectTextureImage> mResizerImage;
  nsAutoPtr<RectTextureImage> mCornerMaskImage;
  nsAutoPtr<RectTextureImage> mTitlebarImage;
  nsAutoPtr<RectTextureImage> mBasicCompositorImage;

  
  
  nsIntRegion           mDirtyTitlebarRegion;

  nsIntRegion           mDraggableRegion;

  
  
  
  
  mutable CGFloat       mBackingScaleFactor;

  bool                  mVisible;
  bool                  mDrawing;
  bool                  mPluginDrawing;
  bool                  mIsDispatchPaint; 

  NP_CGContext          mPluginCGContext;
  nsIPluginInstanceOwner* mPluginInstanceOwner; 

  
  
  nsAutoPtr<GLPresenter> mGLPresenter;

  nsRefPtr<APZCTreeManager> mAPZCTreeManager;

  mozilla::UniquePtr<mozilla::VibrancyManager> mVibrancyManager;

  static uint32_t sLastInputEventCount;

  void ReleaseTitlebarCGContext();
};

void NS_InstallPluginKeyEventsHandler();
void NS_RemovePluginKeyEventsHandler();

#endif 
