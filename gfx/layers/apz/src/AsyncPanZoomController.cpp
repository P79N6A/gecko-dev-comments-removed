





#include <math.h>                       
#include <stdint.h>                     
#include <sys/types.h>                  
#include <algorithm>                    
#include "AsyncPanZoomController.h"     
#include "Compositor.h"                 
#include "CompositorParent.h"           
#include "FrameMetrics.h"               
#include "GestureEventListener.h"       
#include "InputData.h"                  
#include "InputBlockState.h"            
#include "OverscrollHandoffChain.h"     
#include "Units.h"                      
#include "UnitTransforms.h"             
#include "base/message_loop.h"          
#include "base/task.h"                  
#include "base/tracked.h"               
#include "gfxPrefs.h"                   
#include "gfxTypes.h"                   
#include "mozilla/Assertions.h"         
#include "mozilla/BasicEvents.h"        
#include "mozilla/ClearOnShutdown.h"    
#include "mozilla/Constants.h"          
#include "mozilla/EventForwards.h"      
#include "mozilla/Preferences.h"        
#include "mozilla/ReentrantMonitor.h"   
#include "mozilla/StaticPtr.h"          
#include "mozilla/TimeStamp.h"          
#include "mozilla/dom/AnimationPlayer.h" 
#include "mozilla/dom/Touch.h"          
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/gfx/BaseRect.h"       
#include "mozilla/gfx/Point.h"          
#include "mozilla/gfx/Rect.h"           
#include "mozilla/gfx/ScaleFactor.h"    
#include "mozilla/layers/APZCTreeManager.h"  
#include "mozilla/layers/AsyncCompositionManager.h"  
#include "mozilla/layers/Axis.h"        
#include "mozilla/layers/AxisPhysicsModel.h" 
#include "mozilla/layers/AxisPhysicsMSDModel.h" 
#include "mozilla/layers/LayerTransactionParent.h" 
#include "mozilla/layers/PCompositorParent.h" 
#include "mozilla/layers/TaskThrottler.h"  
#include "mozilla/mozalloc.h"           
#include "mozilla/unused.h"             
#include "mozilla/FloatingPoint.h"      
#include "nsAlgorithm.h"                
#include "nsAutoPtr.h"                  
#include "nsCOMPtr.h"                   
#include "nsDebug.h"                    
#include "nsIDOMWindowUtils.h"          
#include "nsISupportsImpl.h"            
#include "nsMathUtils.h"                
#include "nsPoint.h"                    
#include "nsStyleConsts.h"
#include "nsStyleStruct.h"              
#include "nsTArray.h"                   
#include "nsThreadUtils.h"              
#include "SharedMemoryBasic.h"          



#define ENABLE_APZC_LOGGING 0


#if ENABLE_APZC_LOGGING
#  include "LayersLogging.h"
#  define APZC_LOG(...) printf_stderr("APZC: " __VA_ARGS__)
#  define APZC_LOG_FM(fm, prefix, ...) \
    { std::stringstream ss; \
      ss << nsPrintfCString(prefix, __VA_ARGS__).get(); \
      AppendToString(ss, fm, ":", "", true); \
      APZC_LOG("%s", ss.str().c_str()); \
    }
#else
#  define APZC_LOG(...)
#  define APZC_LOG_FM(fm, prefix, ...)
#endif


namespace {

int32_t
WidgetModifiersToDOMModifiers(mozilla::Modifiers aModifiers)
{
  int32_t result = 0;
  if (aModifiers & mozilla::MODIFIER_SHIFT) {
    result |= nsIDOMWindowUtils::MODIFIER_SHIFT;
  }
  if (aModifiers & mozilla::MODIFIER_CONTROL) {
    result |= nsIDOMWindowUtils::MODIFIER_CONTROL;
  }
  if (aModifiers & mozilla::MODIFIER_ALT) {
    result |= nsIDOMWindowUtils::MODIFIER_ALT;
  }
  if (aModifiers & mozilla::MODIFIER_META) {
    result |= nsIDOMWindowUtils::MODIFIER_META;
  }
  if (aModifiers & mozilla::MODIFIER_ALTGRAPH) {
    result |= nsIDOMWindowUtils::MODIFIER_ALTGRAPH;
  }
  if (aModifiers & mozilla::MODIFIER_CAPSLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_CAPSLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_FN) {
    result |= nsIDOMWindowUtils::MODIFIER_FN;
  }
  if (aModifiers & mozilla::MODIFIER_NUMLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_NUMLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_SCROLLLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_SCROLLLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_SYMBOLLOCK) {
    result |= nsIDOMWindowUtils::MODIFIER_SYMBOLLOCK;
  }
  if (aModifiers & mozilla::MODIFIER_OS) {
    result |= nsIDOMWindowUtils::MODIFIER_OS;
  }
  return result;
}

}

namespace mozilla {
namespace layers {

typedef mozilla::layers::AllowedTouchBehavior AllowedTouchBehavior;
typedef GeckoContentController::APZStateChange APZStateChange;
typedef mozilla::gfx::Point Point;
typedef mozilla::gfx::Matrix4x4 Matrix4x4;



















































































































































































































StaticAutoPtr<ComputedTimingFunction> gComputedTimingFunction;




static const CSSToScreenScale MAX_ZOOM(8.0f);




static const CSSToScreenScale MIN_ZOOM(0.125f);






static bool IsCloseToHorizontal(float aAngle, float aThreshold)
{
  return (aAngle < aThreshold || aAngle > (M_PI - aThreshold));
}


static bool IsCloseToVertical(float aAngle, float aThreshold)
{
  return (fabs(aAngle - (M_PI / 2)) < aThreshold);
}

template <typename Units>
static bool IsZero(const gfx::PointTyped<Units>& aPoint)
{
  return FuzzyEqualsMultiplicative(aPoint.x, 0.0f)
      && FuzzyEqualsMultiplicative(aPoint.y, 0.0f);
}

static inline void LogRendertraceRect(const ScrollableLayerGuid& aGuid, const char* aDesc, const char* aColor, const CSSRect& aRect)
{
#ifdef APZC_ENABLE_RENDERTRACE
  static const TimeStamp sRenderStart = TimeStamp::Now();
  TimeDuration delta = TimeStamp::Now() - sRenderStart;
  printf_stderr("(%llu,%lu,%llu)%s RENDERTRACE %f rect %s %f %f %f %f\n",
    aGuid.mLayersId, aGuid.mPresShellId, aGuid.mScrollId,
    aDesc, delta.ToMilliseconds(), aColor,
    aRect.x, aRect.y, aRect.width, aRect.height);
#endif
}

static TimeStamp sFrameTime;
static bool sThreadAssertionsEnabled = true;
static PRThread* sControllerThread;


static uint32_t sAsyncPanZoomControllerCount = 0;

static TimeStamp
GetFrameTime() {
  if (sFrameTime.IsNull()) {
    return TimeStamp::Now();
  }
  return sFrameTime;
}

class MOZ_STACK_CLASS StateChangeNotificationBlocker {
public:
  StateChangeNotificationBlocker(AsyncPanZoomController* aApzc)
    : mApzc(aApzc)
  {
    ReentrantMonitorAutoEnter lock(mApzc->mMonitor);
    mInitialState = mApzc->mState;
    mApzc->mNotificationBlockers++;
  }

  ~StateChangeNotificationBlocker()
  {
    AsyncPanZoomController::PanZoomState newState;
    {
      ReentrantMonitorAutoEnter lock(mApzc->mMonitor);
      mApzc->mNotificationBlockers--;
      newState = mApzc->mState;
    }
    mApzc->DispatchStateChangeNotification(mInitialState, newState);
  }

private:
  AsyncPanZoomController* mApzc;
  AsyncPanZoomController::PanZoomState mInitialState;
};

class FlingAnimation: public AsyncPanZoomAnimation {
public:
  FlingAnimation(AsyncPanZoomController& aApzc,
                 const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                 bool aApplyAcceleration,
                 bool aAllowOverscroll)
    : AsyncPanZoomAnimation(TimeDuration::FromMilliseconds(gfxPrefs::APZFlingRepaintInterval()))
    , mApzc(aApzc)
    , mOverscrollHandoffChain(aOverscrollHandoffChain)
    , mAllowOverscroll(aAllowOverscroll)
  {
    MOZ_ASSERT(mOverscrollHandoffChain);
    TimeStamp now = GetFrameTime();
    ScreenPoint velocity(mApzc.mX.GetVelocity(), mApzc.mY.GetVelocity());

    
    
    
    
    
    
    
    if (aApplyAcceleration && !mApzc.mLastFlingTime.IsNull()
        && (now - mApzc.mLastFlingTime).ToMilliseconds() < gfxPrefs::APZFlingAccelInterval()) {
      if (SameDirection(velocity.x, mApzc.mLastFlingVelocity.x)) {
        velocity.x = Accelerate(velocity.x, mApzc.mLastFlingVelocity.x);
        APZC_LOG("%p Applying fling x-acceleration from %f to %f (delta %f)\n",
                 &mApzc, mApzc.mX.GetVelocity(), velocity.x, mApzc.mLastFlingVelocity.x);
        mApzc.mX.SetVelocity(velocity.x);
      }
      if (SameDirection(velocity.y, mApzc.mLastFlingVelocity.y)) {
        velocity.y = Accelerate(velocity.y, mApzc.mLastFlingVelocity.y);
        APZC_LOG("%p Applying fling y-acceleration from %f to %f (delta %f)\n",
                 &mApzc, mApzc.mY.GetVelocity(), velocity.y, mApzc.mLastFlingVelocity.y);
        mApzc.mY.SetVelocity(velocity.y);
      }
    }

    mApzc.mLastFlingTime = now;
    mApzc.mLastFlingVelocity = velocity;
  }

  





  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) MOZ_OVERRIDE
  {
    
    
    
    
    
    
    
    
    if (aDelta.ToMilliseconds() <= 0) {
      return true;
    }

    bool overscrolled = mApzc.IsOverscrolled();
    float friction = overscrolled ? gfxPrefs::APZOverscrollFlingFriction()
                                  : gfxPrefs::APZFlingFriction();
    float threshold = overscrolled ? gfxPrefs::APZOverscrollFlingStoppedThreshold()
                                   : gfxPrefs::APZFlingStoppedThreshold();

    bool shouldContinueFlingX = mApzc.mX.FlingApplyFrictionOrCancel(aDelta, friction, threshold),
         shouldContinueFlingY = mApzc.mY.FlingApplyFrictionOrCancel(aDelta, friction, threshold);
    
    if (!shouldContinueFlingX && !shouldContinueFlingY) {
      APZC_LOG("%p ending fling animation. overscrolled=%d\n", &mApzc, mApzc.IsOverscrolled());
      
      
      
      
      
      
      
      mDeferredTasks.append(NewRunnableMethod(mOverscrollHandoffChain.get(),
                                              &OverscrollHandoffChain::SnapBackOverscrolledApzc));
      return false;
    }

    
    
    
    
    ScreenPoint velocity(mApzc.mX.GetVelocity(), mApzc.mY.GetVelocity());

    ScreenPoint offset = velocity * aDelta.ToMilliseconds();

    
    
    CSSPoint cssOffset = offset / aFrameMetrics.GetZoom();

    
    
    
    
    CSSPoint overscroll;
    CSSPoint adjustedOffset;
    mApzc.mX.AdjustDisplacement(cssOffset.x, adjustedOffset.x, overscroll.x);
    mApzc.mY.AdjustDisplacement(cssOffset.y, adjustedOffset.y, overscroll.y);

    aFrameMetrics.ScrollBy(adjustedOffset);

    
    if (!IsZero(overscroll)) {
      if (mAllowOverscroll) {
        

        mApzc.OverscrollBy(overscroll);

        
        
        mApzc.mX.SetVelocity(velocity.x);
        mApzc.mY.SetVelocity(velocity.y);

      } else {
        
        

        
        
        
        if (FuzzyEqualsAdditive(overscroll.x, 0.0f, COORDINATE_EPSILON)) {
          velocity.x = 0;
        } else if (FuzzyEqualsAdditive(overscroll.y, 0.0f, COORDINATE_EPSILON)) {
          velocity.y = 0;
        }

        
        
        
        
        
        
        
        
        
        
        
        
        mDeferredTasks.append(NewRunnableMethod(&mApzc,
                                                &AsyncPanZoomController::HandleFlingOverscroll,
                                                velocity,
                                                mOverscrollHandoffChain));

        return false;
      }
    }

    return true;
  }

private:
  static bool SameDirection(float aVelocity1, float aVelocity2)
  {
    return (aVelocity1 == 0.0f)
        || (aVelocity2 == 0.0f)
        || (IsNegative(aVelocity1) == IsNegative(aVelocity2));
  }

  static float Accelerate(float aBase, float aSupplemental)
  {
    return (aBase * gfxPrefs::APZFlingAccelBaseMultiplier())
         + (aSupplemental * gfxPrefs::APZFlingAccelSupplementalMultiplier());
  }

  AsyncPanZoomController& mApzc;
  nsRefPtr<const OverscrollHandoffChain> mOverscrollHandoffChain;
  bool mAllowOverscroll;
};

class ZoomAnimation: public AsyncPanZoomAnimation {
public:
  ZoomAnimation(CSSPoint aStartOffset, CSSToScreenScale aStartZoom,
                CSSPoint aEndOffset, CSSToScreenScale aEndZoom)
    : mTotalDuration(TimeDuration::FromMilliseconds(gfxPrefs::APZZoomAnimationDuration()))
    , mStartOffset(aStartOffset)
    , mStartZoom(aStartZoom)
    , mEndOffset(aEndOffset)
    , mEndZoom(aEndZoom)
  {}

  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) MOZ_OVERRIDE
  {
    mDuration += aDelta;
    double animPosition = mDuration / mTotalDuration;

    if (animPosition >= 1.0) {
      aFrameMetrics.SetZoom(mEndZoom);
      aFrameMetrics.SetScrollOffset(mEndOffset);
      return false;
    }

    
    
    float sampledPosition = gComputedTimingFunction->GetValue(animPosition);

    
    
    aFrameMetrics.SetZoom(CSSToScreenScale(1 /
      (sampledPosition / mEndZoom.scale +
      (1 - sampledPosition) / mStartZoom.scale)));

    aFrameMetrics.SetScrollOffset(CSSPoint::FromUnknownPoint(gfx::Point(
      mEndOffset.x * sampledPosition + mStartOffset.x * (1 - sampledPosition),
      mEndOffset.y * sampledPosition + mStartOffset.y * (1 - sampledPosition)
    )));

    return true;
  }

private:
  TimeDuration mDuration;
  const TimeDuration mTotalDuration;

  
  
  
  
  CSSPoint mStartOffset;
  CSSToScreenScale mStartZoom;

  
  
  
  CSSPoint mEndOffset;
  CSSToScreenScale mEndZoom;
};

class OverscrollSnapBackAnimation: public AsyncPanZoomAnimation {
public:
  explicit OverscrollSnapBackAnimation(AsyncPanZoomController& aApzc)
    : mApzc(aApzc)
  {
    
    
    
    mApzc.mX.SetVelocity(0);
    mApzc.mY.SetVelocity(0);
  }

  virtual bool Sample(FrameMetrics& aFrameMetrics,
                      const TimeDuration& aDelta) MOZ_OVERRIDE
  {
    
    bool continueX = mApzc.mX.SampleSnapBack(aDelta);
    bool continueY = mApzc.mY.SampleSnapBack(aDelta);
    return continueX || continueY;
  }
private:
  AsyncPanZoomController& mApzc;
};

class SmoothScrollAnimation : public AsyncPanZoomAnimation {
public:
  SmoothScrollAnimation(AsyncPanZoomController& aApzc,
                        const nsPoint &aInitialPosition,
                        const nsPoint &aInitialVelocity,
                        const nsPoint& aDestination, double aSpringConstant,
                        double aDampingRatio)
   : AsyncPanZoomAnimation(TimeDuration::FromMilliseconds(
                           gfxPrefs::APZSmoothScrollRepaintInterval()))
   , mApzc(aApzc)
   , mXAxisModel(aInitialPosition.x, aDestination.x, aInitialVelocity.x,
                 aSpringConstant, aDampingRatio)
   , mYAxisModel(aInitialPosition.y, aDestination.y, aInitialVelocity.y,
                 aSpringConstant, aDampingRatio)
  {
  }

  





  bool Sample(FrameMetrics& aFrameMetrics, const TimeDuration& aDelta) {

    if (aDelta.ToMilliseconds() <= 0) {
      return true;
    }

    if (mXAxisModel.IsFinished() && mYAxisModel.IsFinished()) {
      return false;
    }

    mXAxisModel.Simulate(aDelta);
    mYAxisModel.Simulate(aDelta);

    CSSPoint position = CSSPoint::FromAppUnits(nsPoint(mXAxisModel.GetPosition(),
                                                       mYAxisModel.GetPosition()));
    CSSPoint css_velocity = CSSPoint::FromAppUnits(nsPoint(mXAxisModel.GetVelocity(),
                                                           mYAxisModel.GetVelocity()));

    
    ScreenPoint velocity = ScreenPoint(css_velocity.x, css_velocity.y) / 1000.0f;

    
    
    if (mXAxisModel.IsFinished()) {
      mApzc.mX.SetVelocity(0);
    } else {
      mApzc.mX.SetVelocity(velocity.x);
    }
    if (mYAxisModel.IsFinished()) {
      mApzc.mY.SetVelocity(0);
    } else {
      mApzc.mY.SetVelocity(velocity.y);
    }

    
    
    float displacement_x = position.x - mApzc.mX.GetOrigin();
    float displacement_y = position.y - mApzc.mY.GetOrigin();

    CSSPoint overscroll;
    CSSPoint adjustedOffset;
    mApzc.mX.AdjustDisplacement(displacement_x, adjustedOffset.x, overscroll.x);
    mApzc.mY.AdjustDisplacement(displacement_y, adjustedOffset.y, overscroll.y);

    aFrameMetrics.ScrollBy(adjustedOffset);

    
    
    
    
    if (!IsZero(overscroll)) {

      
      

      
      
      
      if (FuzzyEqualsAdditive(overscroll.x, 0.0f, COORDINATE_EPSILON)) {
        velocity.x = 0;
      } else if (FuzzyEqualsAdditive(overscroll.y, 0.0f, COORDINATE_EPSILON)) {
        velocity.y = 0;
      }

      
      
      
      
      
      
      
      
      
      
      
      
      mDeferredTasks.append(NewRunnableMethod(&mApzc,
                                              &AsyncPanZoomController::HandleSmoothScrollOverscroll,
                                              velocity));

      return false;
    }

    return true;
  }
private:
  AsyncPanZoomController& mApzc;
  AxisPhysicsMSDModel mXAxisModel, mYAxisModel;
};

void
AsyncPanZoomController::SetFrameTime(const TimeStamp& aTime) {
  sFrameTime = aTime;
}

void
AsyncPanZoomController::SetThreadAssertionsEnabled(bool aEnabled) {
  sThreadAssertionsEnabled = aEnabled;
}

bool
AsyncPanZoomController::GetThreadAssertionsEnabled() {
  return sThreadAssertionsEnabled;
}

void
AsyncPanZoomController::AssertOnControllerThread() {
  if (!GetThreadAssertionsEnabled()) {
    return;
  }

  static bool sControllerThreadDetermined = false;
  if (!sControllerThreadDetermined) {
    
    
    
    
    sControllerThread = PR_GetCurrentThread();
    sControllerThreadDetermined = true;
  }
  MOZ_ASSERT(sControllerThread == PR_GetCurrentThread());
}

void
AsyncPanZoomController::AssertOnCompositorThread()
{
  if (GetThreadAssertionsEnabled()) {
    Compositor::AssertOnCompositorThread();
  }
}

 void
AsyncPanZoomController::InitializeGlobalState()
{
  MOZ_ASSERT(NS_IsMainThread());

  static bool sInitialized = false;
  if (sInitialized)
    return;
  sInitialized = true;

  gComputedTimingFunction = new ComputedTimingFunction();
  gComputedTimingFunction->Init(
    nsTimingFunction(NS_STYLE_TRANSITION_TIMING_FUNCTION_EASE));
  ClearOnShutdown(&gComputedTimingFunction);
}

AsyncPanZoomController::AsyncPanZoomController(uint64_t aLayersId,
                                               APZCTreeManager* aTreeManager,
                                               GeckoContentController* aGeckoContentController,
                                               GestureBehavior aGestures)
  :  mLayersId(aLayersId),
     mPaintThrottler(GetFrameTime(), TimeDuration::FromMilliseconds(500)),
     mGeckoContentController(aGeckoContentController),
     mRefPtrMonitor("RefPtrMonitor"),
     mSharingFrameMetricsAcrossProcesses(false),
     mMonitor("AsyncPanZoomController"),
     mX(MOZ_THIS_IN_INITIALIZER_LIST()),
     mY(MOZ_THIS_IN_INITIALIZER_LIST()),
     mPanDirRestricted(false),
     mZoomConstraints(false, false, MIN_ZOOM, MAX_ZOOM),
     mLastSampleTime(GetFrameTime()),
     mLastAsyncScrollTime(GetFrameTime()),
     mLastAsyncScrollOffset(0, 0),
     mCurrentAsyncScrollOffset(0, 0),
     mAsyncScrollTimeoutTask(nullptr),
     mState(NOTHING),
     mNotificationBlockers(0),
     mTouchBlockBalance(0),
     mTreeManager(aTreeManager),
     mAPZCId(sAsyncPanZoomControllerCount++),
     mSharedLock(nullptr),
     mAsyncTransformAppliedToContent(false)
{
  MOZ_COUNT_CTOR(AsyncPanZoomController);

  if (aGestures == USE_GESTURE_DETECTOR) {
    mGestureEventListener = new GestureEventListener(this);
  }
}

AsyncPanZoomController::~AsyncPanZoomController() {
  MOZ_COUNT_DTOR(AsyncPanZoomController);
}

PCompositorParent*
AsyncPanZoomController::GetSharedFrameMetricsCompositor()
{
  AssertOnCompositorThread();

  if (mSharingFrameMetricsAcrossProcesses) {
    const CompositorParent::LayerTreeState* state = CompositorParent::GetIndirectShadowTree(mLayersId);
    
    return state ? state->mCrossProcessParent : nullptr;
  }
  return mCompositorParent.get();
}

already_AddRefed<GeckoContentController>
AsyncPanZoomController::GetGeckoContentController() const {
  MonitorAutoLock lock(mRefPtrMonitor);
  nsRefPtr<GeckoContentController> controller = mGeckoContentController;
  return controller.forget();
}

already_AddRefed<GestureEventListener>
AsyncPanZoomController::GetGestureEventListener() const {
  MonitorAutoLock lock(mRefPtrMonitor);
  nsRefPtr<GestureEventListener> listener = mGestureEventListener;
  return listener.forget();
}

void
AsyncPanZoomController::Destroy()
{
  AssertOnCompositorThread();

  CancelAnimation();

  mTouchBlockQueue.Clear();

  { 
    MonitorAutoLock lock(mRefPtrMonitor);
    mGeckoContentController = nullptr;
    mGestureEventListener = nullptr;
  }
  mPrevSibling = nullptr;
  mLastChild = nullptr;
  mParent = nullptr;
  mTreeManager = nullptr;

  PCompositorParent* compositor = GetSharedFrameMetricsCompositor();
  
  if (compositor && mSharedFrameMetricsBuffer) {
    unused << compositor->SendReleaseSharedCompositorFrameMetrics(mFrameMetrics.GetScrollId(), mAPZCId);
  }

  { 
    ReentrantMonitorAutoEnter lock(mMonitor);
    mSharedFrameMetricsBuffer = nullptr;
    delete mSharedLock;
    mSharedLock = nullptr;
  }
}

bool
AsyncPanZoomController::IsDestroyed() const
{
  return mTreeManager == nullptr;
}

float
AsyncPanZoomController::GetTouchStartTolerance()
{
  return (gfxPrefs::APZTouchStartTolerance() * APZCTreeManager::GetDPI());
}

AsyncPanZoomController::AxisLockMode AsyncPanZoomController::GetAxisLockMode()
{
  return static_cast<AxisLockMode>(gfxPrefs::APZAxisLockMode());
}

bool
AsyncPanZoomController::ArePointerEventsConsumable(TouchBlockState* aBlock, uint32_t aTouchPoints) {
  if (aTouchPoints == 0) {
    
    return false;
  }

  
  
  
  
  
  

  bool pannable = aBlock->GetOverscrollHandoffChain()->CanBePanned(this);
  bool zoomable = mZoomConstraints.mAllowZoom;

  pannable &= (aBlock->TouchActionAllowsPanningX() || aBlock->TouchActionAllowsPanningY());
  zoomable &= (aBlock->TouchActionAllowsPinchZoom());

  
  
  bool consumable = (aTouchPoints == 1 ? pannable : zoomable);
  if (!consumable) {
    return false;
  }

  return true;
}

nsEventStatus AsyncPanZoomController::ReceiveInputEvent(const InputData& aEvent) {
  AssertOnControllerThread();

  if (aEvent.mInputType != MULTITOUCH_INPUT) {
    HandleInputEvent(aEvent);
    
    
    return nsEventStatus_eConsumeDoDefault;
  }

  TouchBlockState* block = nullptr;
  if (aEvent.AsMultiTouchInput().mType == MultiTouchInput::MULTITOUCH_START) {
    block = StartNewTouchBlock(false);
    APZC_LOG("%p started new touch block %p\n", this, block);

    
    
    
    
    
    
    if (block == CurrentTouchBlock()) {
      if (GetVelocityVector().Length() > gfxPrefs::APZFlingStopOnTapThreshold()) {
        
        
        block->DisallowSingleTap();
      }
      block->GetOverscrollHandoffChain()->CancelAnimations();
    }

    if (mFrameMetrics.mMayHaveTouchListeners || mFrameMetrics.mMayHaveTouchCaret) {
      
      
      ScheduleContentResponseTimeout();
    } else {
      
      
      
      APZC_LOG("%p not waiting for content response on block %p\n", this, block);
      mTouchBlockBalance++;
      block->TimeoutContentResponse();
    }
  } else if (mTouchBlockQueue.IsEmpty()) {
    NS_WARNING("Received a non-start touch event while no touch blocks active!");
  } else {
    
    block = mTouchBlockQueue.LastElement().get();
    APZC_LOG("%p received new event in block %p\n", this, block);
  }

  if (!block) {
    return nsEventStatus_eIgnore;
  }

  nsEventStatus result = ArePointerEventsConsumable(block, aEvent.AsMultiTouchInput().mTouches.Length())
      ? nsEventStatus_eConsumeDoDefault
      : nsEventStatus_eIgnore;

  if (block == CurrentTouchBlock() && block->IsReadyForHandling()) {
    APZC_LOG("%p's current touch block is ready with preventdefault %d\n",
        this, block->IsDefaultPrevented());
    if (block->IsDefaultPrevented()) {
      return result;
    }
    HandleInputEvent(aEvent);
    return result;
  }

  
  block->AddEvent(aEvent.AsMultiTouchInput());
  return result;
}

nsEventStatus AsyncPanZoomController::HandleInputEvent(const InputData& aEvent) {
  AssertOnControllerThread();

  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (aEvent.mInputType) {
  case MULTITOUCH_INPUT: {
    const MultiTouchInput& multiTouchInput = aEvent.AsMultiTouchInput();

    nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
    if (listener) {
      rv = listener->HandleInputEvent(multiTouchInput);
      if (rv == nsEventStatus_eConsumeNoDefault) {
        return rv;
      }
    }

    switch (multiTouchInput.mType) {
      case MultiTouchInput::MULTITOUCH_START: rv = OnTouchStart(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_MOVE: rv = OnTouchMove(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_END: rv = OnTouchEnd(multiTouchInput); break;
      case MultiTouchInput::MULTITOUCH_CANCEL: rv = OnTouchCancel(multiTouchInput); break;
      default: NS_WARNING("Unhandled multitouch"); break;
    }
    break;
  }
  case PANGESTURE_INPUT: {
    const PanGestureInput& panGestureInput = aEvent.AsPanGestureInput();
    switch (panGestureInput.mType) {
      case PanGestureInput::PANGESTURE_MAYSTART: rv = OnPanMayBegin(panGestureInput); break;
      case PanGestureInput::PANGESTURE_CANCELLED: rv = OnPanCancelled(panGestureInput); break;
      case PanGestureInput::PANGESTURE_START: rv = OnPanBegin(panGestureInput); break;
      case PanGestureInput::PANGESTURE_PAN: rv = OnPan(panGestureInput, true); break;
      case PanGestureInput::PANGESTURE_END: rv = OnPanEnd(panGestureInput); break;
      case PanGestureInput::PANGESTURE_MOMENTUMSTART: rv = OnPanMomentumStart(panGestureInput); break;
      case PanGestureInput::PANGESTURE_MOMENTUMPAN: rv = OnPan(panGestureInput, false); break;
      case PanGestureInput::PANGESTURE_MOMENTUMEND: rv = OnPanMomentumEnd(panGestureInput); break;
      default: NS_WARNING("Unhandled pan gesture"); break;
    }
    break;
  }
  default: NS_WARNING("Unhandled input event"); break;
  }

  return rv;
}

nsEventStatus AsyncPanZoomController::HandleGestureEvent(const InputData& aEvent)
{
  AssertOnControllerThread();

  nsEventStatus rv = nsEventStatus_eIgnore;

  switch (aEvent.mInputType) {
  case PINCHGESTURE_INPUT: {
    const PinchGestureInput& pinchGestureInput = aEvent.AsPinchGestureInput();
    switch (pinchGestureInput.mType) {
      case PinchGestureInput::PINCHGESTURE_START: rv = OnScaleBegin(pinchGestureInput); break;
      case PinchGestureInput::PINCHGESTURE_SCALE: rv = OnScale(pinchGestureInput); break;
      case PinchGestureInput::PINCHGESTURE_END: rv = OnScaleEnd(pinchGestureInput); break;
      default: NS_WARNING("Unhandled pinch gesture"); break;
    }
    break;
  }
  case TAPGESTURE_INPUT: {
    const TapGestureInput& tapGestureInput = aEvent.AsTapGestureInput();
    switch (tapGestureInput.mType) {
      case TapGestureInput::TAPGESTURE_LONG: rv = OnLongPress(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_LONG_UP: rv = OnLongPressUp(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_UP: rv = OnSingleTapUp(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_CONFIRMED: rv = OnSingleTapConfirmed(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_DOUBLE: rv = OnDoubleTap(tapGestureInput); break;
      case TapGestureInput::TAPGESTURE_CANCEL: rv = OnCancelTap(tapGestureInput); break;
      default: NS_WARNING("Unhandled tap gesture"); break;
    }
    break;
  }
  default: NS_WARNING("Unhandled input event"); break;
  }

  return rv;
}

nsEventStatus AsyncPanZoomController::OnTouchStart(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-start in state %d\n", this, mState);
  mPanDirRestricted = false;
  ScreenPoint point = GetFirstTouchScreenPoint(aEvent);

  switch (mState) {
    case FLING:
    case ANIMATING_ZOOM:
    case SMOOTH_SCROLL:
      CurrentTouchBlock()->GetOverscrollHandoffChain()->CancelAnimations();
      
    case NOTHING: {
      mX.StartTouch(point.x, aEvent.mTime);
      mY.StartTouch(point.y, aEvent.mTime);
      if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
        controller->NotifyAPZStateChange(
            GetGuid(), APZStateChange::StartTouch,
            CurrentTouchBlock()->GetOverscrollHandoffChain()->CanBePanned(this));
      }
      SetState(TOUCHING);
      break;
    }
    case TOUCHING:
    case PANNING:
    case PANNING_LOCKED_X:
    case PANNING_LOCKED_Y:
    case CROSS_SLIDING_X:
    case CROSS_SLIDING_Y:
    case PINCHING:
      NS_WARNING("Received impossible touch in OnTouchStart");
      break;
    default:
      NS_WARNING("Unhandled case in OnTouchStart");
      break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchMove(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-move in state %d\n", this, mState);
  switch (mState) {
    case FLING:
    case SMOOTH_SCROLL:
    case NOTHING:
    case ANIMATING_ZOOM:
      
      
      return nsEventStatus_eIgnore;

    case CROSS_SLIDING_X:
    case CROSS_SLIDING_Y:
      
      
      return nsEventStatus_eIgnore;

    case TOUCHING: {
      float panThreshold = GetTouchStartTolerance();
      UpdateWithTouchAtDevicePoint(aEvent);

      if (PanDistance() < panThreshold) {
        return nsEventStatus_eIgnore;
      }

      if (gfxPrefs::TouchActionEnabled() && CurrentTouchBlock()->TouchActionAllowsPanningXY()) {
        
        
        
        
        StartPanning(aEvent);
        return nsEventStatus_eConsumeNoDefault;
      }

      return StartPanning(aEvent);
    }

    case PANNING:
    case PANNING_LOCKED_X:
    case PANNING_LOCKED_Y:
      TrackTouch(aEvent);
      return nsEventStatus_eConsumeNoDefault;

    case PINCHING:
      
      NS_WARNING("Gesture listener should have handled pinching in OnTouchMove.");
      return nsEventStatus_eIgnore;

    case SNAP_BACK:  
                     
                     
      NS_WARNING("Received impossible touch in OnTouchMove");
      break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchEnd(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-end in state %d\n", this, mState);

  OnTouchEndOrCancel();

  
  
  
  
  
  if (mState != NOTHING) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    SendAsyncScrollEvent();
  }

  switch (mState) {
  case FLING:
    
    NS_WARNING("Received impossible touch end in OnTouchEnd.");
    
  case ANIMATING_ZOOM:
  case SMOOTH_SCROLL:
  case NOTHING:
    
    
    return nsEventStatus_eIgnore;

  case TOUCHING:
  case CROSS_SLIDING_X:
  case CROSS_SLIDING_Y:
    SetState(NOTHING);
    return nsEventStatus_eIgnore;

  case PANNING:
  case PANNING_LOCKED_X:
  case PANNING_LOCKED_Y:
  {
    CurrentTouchBlock()->GetOverscrollHandoffChain()->FlushRepaints();
    mX.EndTouch(aEvent.mTime);
    mY.EndTouch(aEvent.mTime);
    ScreenPoint flingVelocity(mX.GetVelocity(), mY.GetVelocity());
    
    
    
    mX.SetVelocity(0);
    mY.SetVelocity(0);
    
    
    
    
    StateChangeNotificationBlocker blocker(this);
    SetState(NOTHING);
    APZC_LOG("%p starting a fling animation\n", this);
    
    
    
    if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
      treeManagerLocal->DispatchFling(this,
                                      flingVelocity,
                                      CurrentTouchBlock()->GetOverscrollHandoffChain(),
                                      false );
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  case PINCHING:
    SetState(NOTHING);
    
    NS_WARNING("Gesture listener should have handled pinching in OnTouchEnd.");
    return nsEventStatus_eIgnore;

  case SNAP_BACK:  
                   
                   
    NS_WARNING("Received impossible touch in OnTouchEnd");
    break;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnTouchCancel(const MultiTouchInput& aEvent) {
  APZC_LOG("%p got a touch-cancel in state %d\n", this, mState);
  OnTouchEndOrCancel();
  mX.CancelTouch();
  mY.CancelTouch();
  CancelAnimation();
  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleBegin(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-begin in state %d\n", this, mState);

  
  
  if (HasReadyTouchBlock() && !CurrentTouchBlock()->TouchActionAllowsPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  if (!mZoomConstraints.mAllowZoom) {
    return nsEventStatus_eConsumeNoDefault;
  }

  SetState(PINCHING);
  mLastZoomFocus = ToParentLayerCoords(aEvent.mFocusPoint) - mFrameMetrics.mCompositionBounds.TopLeft();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScale(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale in state %d\n", this, mState);

  if (HasReadyTouchBlock() && !CurrentTouchBlock()->TouchActionAllowsPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  if (mState != PINCHING) {
    return nsEventStatus_eConsumeNoDefault;
  }

  float prevSpan = aEvent.mPreviousSpan;
  if (fabsf(prevSpan) <= EPSILON || fabsf(aEvent.mCurrentSpan) <= EPSILON) {
    
    return nsEventStatus_eConsumeNoDefault;
  }

  float spanRatio = aEvent.mCurrentSpan / aEvent.mPreviousSpan;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToParentLayerScale userZoom = mFrameMetrics.GetZoomToParent();
    ParentLayerPoint focusPoint = ToParentLayerCoords(aEvent.mFocusPoint) - mFrameMetrics.mCompositionBounds.TopLeft();
    CSSPoint cssFocusPoint = focusPoint / userZoom;

    CSSPoint focusChange = (mLastZoomFocus - focusPoint) / userZoom;
    
    
    if (mX.DisplacementWillOverscroll(focusChange.x) != Axis::OVERSCROLL_NONE) {
      focusChange.x -= mX.DisplacementWillOverscrollAmount(focusChange.x);
    }
    if (mY.DisplacementWillOverscroll(focusChange.y) != Axis::OVERSCROLL_NONE) {
      focusChange.y -= mY.DisplacementWillOverscrollAmount(focusChange.y);
    }
    ScrollBy(focusChange);

    
    
    
    CSSPoint neededDisplacement;

    CSSToParentLayerScale realMinZoom = mZoomConstraints.mMinZoom * mFrameMetrics.mTransformScale;
    CSSToParentLayerScale realMaxZoom = mZoomConstraints.mMaxZoom * mFrameMetrics.mTransformScale;
    realMinZoom.scale = std::max(realMinZoom.scale,
                                 mFrameMetrics.mCompositionBounds.width / mFrameMetrics.mScrollableRect.width);
    realMinZoom.scale = std::max(realMinZoom.scale,
                                 mFrameMetrics.mCompositionBounds.height / mFrameMetrics.mScrollableRect.height);
    if (realMaxZoom < realMinZoom) {
      realMaxZoom = realMinZoom;
    }

    bool doScale = (spanRatio > 1.0 && userZoom < realMaxZoom) ||
                   (spanRatio < 1.0 && userZoom > realMinZoom);

    if (doScale) {
      spanRatio = clamped(spanRatio,
                          realMinZoom.scale / userZoom.scale,
                          realMaxZoom.scale / userZoom.scale);

      
      
      neededDisplacement.x = -mX.ScaleWillOverscrollAmount(spanRatio, cssFocusPoint.x);
      neededDisplacement.y = -mY.ScaleWillOverscrollAmount(spanRatio, cssFocusPoint.y);

      ScaleWithFocus(spanRatio, cssFocusPoint);

      if (neededDisplacement != CSSPoint()) {
        ScrollBy(neededDisplacement);
      }

      ScheduleComposite();
      
      
      UpdateSharedCompositorFrameMetrics();
    }

    mLastZoomFocus = focusPoint;
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnScaleEnd(const PinchGestureInput& aEvent) {
  APZC_LOG("%p got a scale-end in state %d\n", this, mState);

  if (HasReadyTouchBlock() && !CurrentTouchBlock()->TouchActionAllowsPinchZoom()) {
    return nsEventStatus_eIgnore;
  }

  SetState(NOTHING);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    
    
    
    
    
    
    
    
    if (HasReadyTouchBlock()) {
      CurrentTouchBlock()->GetOverscrollHandoffChain()->ClearOverscroll();
    } else {
      ClearOverscroll();
    }

    ScheduleComposite();
    RequestContentRepaint();
    UpdateSharedCompositorFrameMetrics();
  }

  return nsEventStatus_eConsumeNoDefault;
}

bool
AsyncPanZoomController::ConvertToGecko(const ScreenPoint& aPoint, CSSPoint* aOut)
{
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    Matrix4x4 transformToGecko = treeManagerLocal->GetApzcToGeckoTransform(this);
    Point result = transformToGecko * Point(aPoint.x, aPoint.y);
    
    
    LayoutDevicePoint layoutPoint = LayoutDevicePoint(result.x, result.y);
    { 
      ReentrantMonitorAutoEnter lock(mMonitor);
      *aOut = layoutPoint / mFrameMetrics.mDevPixelsPerCSSPixel;
    }
    return true;
  }
  return false;
}

nsEventStatus AsyncPanZoomController::OnPanMayBegin(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-maybegin in state %d\n", this, mState);

  mX.StartTouch(aEvent.mPanStartPoint.x, aEvent.mTime);
  mY.StartTouch(aEvent.mPanStartPoint.y, aEvent.mTime);
  if (mPanGestureState) {
    mPanGestureState->GetOverscrollHandoffChain()->CancelAnimations();
  } else {
    CancelAnimation();
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanCancelled(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-cancelled in state %d\n", this, mState);

  mX.CancelTouch();
  mY.CancelTouch();

  return nsEventStatus_eConsumeNoDefault;
}


nsEventStatus AsyncPanZoomController::OnPanBegin(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-begin in state %d\n", this, mState);

  if (mState == SMOOTH_SCROLL) {
    
    CancelAnimation();
  }

  mPanGestureState = MakeUnique<InputBlockState>(BuildOverscrollHandoffChain());

  mX.StartTouch(aEvent.mPanStartPoint.x, aEvent.mTime);
  mY.StartTouch(aEvent.mPanStartPoint.y, aEvent.mTime);

  if (GetAxisLockMode() == FREE) {
    SetState(PANNING);
    return nsEventStatus_eConsumeNoDefault;
  }

  float dx = aEvent.mPanDisplacement.x, dy = aEvent.mPanDisplacement.y;
  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  HandlePanning(angle);

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPan(const PanGestureInput& aEvent, bool aFingersOnTouchpad) {
  APZC_LOG("%p got a pan-pan in state %d\n", this, mState);

  if (mState == SMOOTH_SCROLL) {
    if (aEvent.mType == PanGestureInput::PANGESTURE_MOMENTUMPAN) {
      
      
      
      
      
      return nsEventStatus_eConsumeNoDefault;
    } else {
      
      CancelAnimation();
    }
  }

  
  
  
  
  mX.UpdateWithTouchAtDevicePoint(aEvent.mPanStartPoint.x, aEvent.mTime);
  mY.UpdateWithTouchAtDevicePoint(aEvent.mPanStartPoint.y, aEvent.mTime);

  ScreenPoint panDisplacement = aEvent.mPanDisplacement;
  ToGlobalScreenCoordinates(&panDisplacement, aEvent.mPanStartPoint);
  HandlePanningUpdate(panDisplacement);

  
  if (mPanGestureState) {
    CallDispatchScroll(aEvent.mPanStartPoint, aEvent.mPanStartPoint + aEvent.mPanDisplacement,
                       *mPanGestureState->GetOverscrollHandoffChain(), 0);
  }

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanEnd(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-end in state %d\n", this, mState);

  mPanGestureState = nullptr;

  mX.EndTouch(aEvent.mTime);
  mY.EndTouch(aEvent.mTime);
  RequestContentRepaint();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanMomentumStart(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-momentumstart in state %d\n", this, mState);

  if (mState == SMOOTH_SCROLL) {
    
    CancelAnimation();
  }

  mPanGestureState = MakeUnique<InputBlockState>(BuildOverscrollHandoffChain());

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnPanMomentumEnd(const PanGestureInput& aEvent) {
  APZC_LOG("%p got a pan-momentumend in state %d\n", this, mState);

  mPanGestureState = nullptr;

  
  
  
  mX.CancelTouch();
  mY.CancelTouch();

  RequestContentRepaint();

  return nsEventStatus_eConsumeNoDefault;
}

nsEventStatus AsyncPanZoomController::OnLongPress(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-press in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aEvent.mPoint, &geckoScreenPoint)) {
      StartNewTouchBlock(true);
      ScheduleContentResponseTimeout();
      controller->HandleLongTap(geckoScreenPoint, modifiers, GetGuid());
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnLongPressUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a long-tap-up in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aEvent.mPoint, &geckoScreenPoint)) {
      controller->HandleLongTapUp(geckoScreenPoint, modifiers, GetGuid());
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::GenerateSingleTap(const ScreenIntPoint& aPoint, mozilla::Modifiers aModifiers) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    CSSPoint geckoScreenPoint;
    if (ConvertToGecko(aPoint, &geckoScreenPoint)) {
      if (!CurrentTouchBlock()->SetSingleTapOccurred()) {
        return nsEventStatus_eIgnore;
      }
      
      
      
      
      
      controller->PostDelayedTask(
        NewRunnableMethod(controller.get(), &GeckoContentController::HandleSingleTap,
                          geckoScreenPoint, WidgetModifiersToDOMModifiers(aModifiers),
                          GetGuid()),
        0);
      return nsEventStatus_eConsumeNoDefault;
    }
  }
  return nsEventStatus_eIgnore;
}

void AsyncPanZoomController::OnTouchEndOrCancel() {
  if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
    controller->NotifyAPZStateChange(
        GetGuid(), APZStateChange::EndTouch, CurrentTouchBlock()->SingleTapOccurred());
  }
}

nsEventStatus AsyncPanZoomController::OnSingleTapUp(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-up in state %d\n", this, mState);
  
  
  if (!(mZoomConstraints.mAllowDoubleTapZoom && CurrentTouchBlock()->TouchActionAllowsDoubleTapZoom())) {
    return GenerateSingleTap(aEvent.mPoint, aEvent.modifiers);
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnSingleTapConfirmed(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a single-tap-confirmed in state %d\n", this, mState);
  return GenerateSingleTap(aEvent.mPoint, aEvent.modifiers);
}

nsEventStatus AsyncPanZoomController::OnDoubleTap(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a double-tap in state %d\n", this, mState);
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    if (mZoomConstraints.mAllowDoubleTapZoom && CurrentTouchBlock()->TouchActionAllowsDoubleTapZoom()) {
      int32_t modifiers = WidgetModifiersToDOMModifiers(aEvent.modifiers);
      CSSPoint geckoScreenPoint;
      if (ConvertToGecko(aEvent.mPoint, &geckoScreenPoint)) {
        controller->HandleDoubleTap(geckoScreenPoint, modifiers, GetGuid());
      }
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  return nsEventStatus_eIgnore;
}

nsEventStatus AsyncPanZoomController::OnCancelTap(const TapGestureInput& aEvent) {
  APZC_LOG("%p got a cancel-tap in state %d\n", this, mState);
  
  return nsEventStatus_eIgnore;
}



static void TransformVector(const Matrix4x4& aTransform,
                            ScreenPoint* aVector,
                            const ScreenPoint& aAnchor) {
  ScreenPoint start = aAnchor;
  ScreenPoint end = aAnchor + *aVector;
  start = TransformTo<ScreenPixel>(aTransform, start);
  end = TransformTo<ScreenPixel>(aTransform, end);
  *aVector = end - start;
}

void AsyncPanZoomController::ToGlobalScreenCoordinates(ScreenPoint* aVector,
                                                       const ScreenPoint& aAnchor) const {
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    Matrix4x4 transform = treeManagerLocal->GetScreenToApzcTransform(this);
    transform.Invert();
    TransformVector(transform, aVector, aAnchor);
  }
}

void AsyncPanZoomController::ToLocalScreenCoordinates(ScreenPoint* aVector,
                                                      const ScreenPoint& aAnchor) const {
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    Matrix4x4 transform = treeManagerLocal->GetScreenToApzcTransform(this);
    TransformVector(transform, aVector, aAnchor);
  }
}

float AsyncPanZoomController::PanDistance() const {
  ScreenPoint panVector;
  ScreenPoint panStart;
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    panVector = ScreenPoint(mX.PanDistance(), mY.PanDistance());
    panStart = PanStart();
  }
  ToGlobalScreenCoordinates(&panVector, panStart);
  return NS_hypot(panVector.x, panVector.y);
}

ScreenPoint AsyncPanZoomController::PanStart() const {
  return ScreenPoint(mX.PanStart(), mY.PanStart());
}

const ScreenPoint AsyncPanZoomController::GetVelocityVector() {
  return ScreenPoint(mX.GetVelocity(), mY.GetVelocity());
}

void AsyncPanZoomController::HandlePanningWithTouchAction(double aAngle) {
  
  
  if (CurrentTouchBlock()->TouchActionAllowsPanningXY()) {
    if (mX.CanScrollNow() && mY.CanScrollNow()) {
      if (IsCloseToHorizontal(aAngle, gfxPrefs::APZAxisLockAngle())) {
        mY.SetAxisLocked(true);
        SetState(PANNING_LOCKED_X);
      } else if (IsCloseToVertical(aAngle, gfxPrefs::APZAxisLockAngle())) {
        mX.SetAxisLocked(true);
        SetState(PANNING_LOCKED_Y);
      } else {
        SetState(PANNING);
      }
    } else if (mX.CanScrollNow() || mY.CanScrollNow()) {
      SetState(PANNING);
    } else {
      SetState(NOTHING);
    }
  } else if (CurrentTouchBlock()->TouchActionAllowsPanningX()) {
    
    
    if (IsCloseToHorizontal(aAngle, gfxPrefs::APZAllowedDirectPanAngle())) {
      mY.SetAxisLocked(true);
      SetState(PANNING_LOCKED_X);
      mPanDirRestricted = true;
    } else {
      
      
      SetState(NOTHING);
    }
  } else if (CurrentTouchBlock()->TouchActionAllowsPanningY()) {
    if (IsCloseToVertical(aAngle, gfxPrefs::APZAllowedDirectPanAngle())) {
      mX.SetAxisLocked(true);
      SetState(PANNING_LOCKED_Y);
      mPanDirRestricted = true;
    } else {
      SetState(NOTHING);
    }
  } else {
    SetState(NOTHING);
  }
}

void AsyncPanZoomController::HandlePanning(double aAngle) {
  ReentrantMonitorAutoEnter lock(mMonitor);
  if (!gfxPrefs::APZCrossSlideEnabled() && (!mX.CanScrollNow() || !mY.CanScrollNow())) {
    SetState(PANNING);
  } else if (IsCloseToHorizontal(aAngle, gfxPrefs::APZAxisLockAngle())) {
    mY.SetAxisLocked(true);
    if (mX.CanScrollNow()) {
      SetState(PANNING_LOCKED_X);
    } else {
      SetState(CROSS_SLIDING_X);
      mX.SetAxisLocked(true);
    }
  } else if (IsCloseToVertical(aAngle, gfxPrefs::APZAxisLockAngle())) {
    mX.SetAxisLocked(true);
    if (mY.CanScrollNow()) {
      SetState(PANNING_LOCKED_Y);
    } else {
      SetState(CROSS_SLIDING_Y);
      mY.SetAxisLocked(true);
    }
  } else {
    SetState(PANNING);
  }
}

void AsyncPanZoomController::HandlePanningUpdate(const ScreenPoint& aDelta) {
  
  if (GetAxisLockMode() == STICKY && !mPanDirRestricted) {

    double angle = atan2(aDelta.y, aDelta.x); 
    angle = fabs(angle); 

    float breakThreshold = gfxPrefs::APZAxisBreakoutThreshold() * APZCTreeManager::GetDPI();

    if (fabs(aDelta.x) > breakThreshold || fabs(aDelta.y) > breakThreshold) {
      if (mState == PANNING_LOCKED_X || mState == CROSS_SLIDING_X) {
        if (!IsCloseToHorizontal(angle, gfxPrefs::APZAxisBreakoutAngle())) {
          mY.SetAxisLocked(false);
          SetState(PANNING);
        }
      } else if (mState == PANNING_LOCKED_Y || mState == CROSS_SLIDING_Y) {
        if (!IsCloseToVertical(angle, gfxPrefs::APZAxisLockAngle())) {
          mX.SetAxisLocked(false);
          SetState(PANNING);
        }
      }
    }
  }
}

nsEventStatus AsyncPanZoomController::StartPanning(const MultiTouchInput& aEvent) {
  ReentrantMonitorAutoEnter lock(mMonitor);

  ScreenPoint point = GetFirstTouchScreenPoint(aEvent);
  float dx = mX.PanDistance(point.x);
  float dy = mY.PanDistance(point.y);

  
  
  mX.StartTouch(point.x, aEvent.mTime);
  mY.StartTouch(point.y, aEvent.mTime);

  double angle = atan2(dy, dx); 
  angle = fabs(angle); 

  if (gfxPrefs::TouchActionEnabled()) {
    HandlePanningWithTouchAction(angle);
  } else {
    if (GetAxisLockMode() == FREE) {
      SetState(PANNING);
    } else {
      HandlePanning(angle);
    }
  }

  if (IsInPanningState()) {
    if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
      controller->NotifyAPZStateChange(GetGuid(), APZStateChange::StartPanning);
    }
    return nsEventStatus_eConsumeNoDefault;
  }
  
  return nsEventStatus_eIgnore;
}

void AsyncPanZoomController::UpdateWithTouchAtDevicePoint(const MultiTouchInput& aEvent) {
  ScreenPoint point = GetFirstTouchScreenPoint(aEvent);
  mX.UpdateWithTouchAtDevicePoint(point.x, aEvent.mTime);
  mY.UpdateWithTouchAtDevicePoint(point.y, aEvent.mTime);
}

bool AsyncPanZoomController::AttemptScroll(const ScreenPoint& aStartPoint,
                                           const ScreenPoint& aEndPoint,
                                           const OverscrollHandoffChain& aOverscrollHandoffChain,
                                           uint32_t aOverscrollHandoffChainIndex) {

  
  
  
  ScreenPoint displacement = aStartPoint - aEndPoint;

  ScreenPoint overscroll;  
  CSSPoint cssOverscroll;  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    CSSToScreenScale zoom = mFrameMetrics.GetZoom();

    
    
    CSSPoint cssDisplacement = displacement / zoom;

    CSSPoint adjustedDisplacement;
    bool xChanged = mX.AdjustDisplacement(cssDisplacement.x, adjustedDisplacement.x, cssOverscroll.x);
    bool yChanged = mY.AdjustDisplacement(cssDisplacement.y, adjustedDisplacement.y, cssOverscroll.y);
    if (xChanged || yChanged) {
      ScheduleComposite();
    }

    overscroll = cssOverscroll * zoom;

    if (!IsZero(adjustedDisplacement)) {
      ScrollBy(adjustedDisplacement);
      ScheduleCompositeAndMaybeRepaint();
      UpdateSharedCompositorFrameMetrics();
    }
  }

  
  if (IsZero(overscroll)) {
    return true;
  }

  
  
  
  
  
  if (CallDispatchScroll(aEndPoint + overscroll, aEndPoint,
                         aOverscrollHandoffChain, aOverscrollHandoffChainIndex + 1)) {
    return true;
  }

  
  
  
  APZC_LOG("%p taking overscroll during panning\n", this);
  return OverscrollBy(cssOverscroll);
}

bool AsyncPanZoomController::OverscrollBy(const CSSPoint& aOverscroll) {
  if (!gfxPrefs::APZOverscrollEnabled()) {
    return false;
  }

  ReentrantMonitorAutoEnter lock(mMonitor);
  
  
  bool xCanScroll = mX.CanScroll();
  bool yCanScroll = mY.CanScroll();
  if (xCanScroll) {
    mX.OverscrollBy(aOverscroll.x);
  }
  if (yCanScroll) {
    mY.OverscrollBy(aOverscroll.y);
  }
  if (xCanScroll || yCanScroll) {
    ScheduleComposite();
    return true;
  }
  
  
  return false;
}

nsRefPtr<const OverscrollHandoffChain> AsyncPanZoomController::BuildOverscrollHandoffChain() {
  if (APZCTreeManager* treeManagerLocal = GetApzcTreeManager()) {
    return treeManagerLocal->BuildOverscrollHandoffChain(this);
  }

  
  
  OverscrollHandoffChain* result = new OverscrollHandoffChain;
  result->Add(this);
  return result;
}

void AsyncPanZoomController::AcceptFling(const ScreenPoint& aVelocity,
                                         const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                                         bool aHandoff,
                                         bool aAllowOverscroll) {
  
  
  mX.SetVelocity(mX.GetVelocity() + aVelocity.x);
  mY.SetVelocity(mY.GetVelocity() + aVelocity.y);
  SetState(FLING);
  StartAnimation(new FlingAnimation(*this,
      aOverscrollHandoffChain,
      !aHandoff,  
      aAllowOverscroll));
}

bool AsyncPanZoomController::AttemptFling(ScreenPoint aVelocity,
                                          const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain,
                                          bool aHandoff) {
  
  if (IsPannable()) {
    AcceptFling(aVelocity,
                aOverscrollHandoffChain,
                aHandoff,
                false );
    return true;
  }

  return false;
}

void AsyncPanZoomController::HandleFlingOverscroll(const ScreenPoint& aVelocity,
                                                   const nsRefPtr<const OverscrollHandoffChain>& aOverscrollHandoffChain) {
  APZCTreeManager* treeManagerLocal = GetApzcTreeManager();
  if (!(treeManagerLocal && treeManagerLocal->DispatchFling(this,
                                                            aVelocity,
                                                            aOverscrollHandoffChain,
                                                            true ))) {
    
    if (IsPannable()) {
      AcceptFling(aVelocity,
                  aOverscrollHandoffChain,
                  true ,
                  true );
    }
  }
}

void AsyncPanZoomController::HandleSmoothScrollOverscroll(const ScreenPoint& aVelocity) {
  
  
  HandleFlingOverscroll(aVelocity, BuildOverscrollHandoffChain());
}

void AsyncPanZoomController::StartSmoothScroll() {
  SetState(SMOOTH_SCROLL);
  nsPoint initialPosition = CSSPoint::ToAppUnits(mFrameMetrics.GetScrollOffset());
  
  
  nsPoint initialVelocity = CSSPoint::ToAppUnits(CSSPoint(mX.GetVelocity(),
                                                          mY.GetVelocity())) * 1000.0f;
  nsPoint destination = CSSPoint::ToAppUnits(mFrameMetrics.GetSmoothScrollOffset());

  StartAnimation(new SmoothScrollAnimation(*this,
                                           initialPosition, initialVelocity,
                                           destination,
                                           gfxPrefs::ScrollBehaviorSpringConstant(),
                                           gfxPrefs::ScrollBehaviorDampingRatio()));
}

void AsyncPanZoomController::StartSnapBack() {
  SetState(SNAP_BACK);
  StartAnimation(new OverscrollSnapBackAnimation(*this));
}

bool AsyncPanZoomController::CallDispatchScroll(const ScreenPoint& aStartPoint,
                                                const ScreenPoint& aEndPoint,
                                                const OverscrollHandoffChain& aOverscrollHandoffChain,
                                                uint32_t aOverscrollHandoffChainIndex) {
  
  
  
  APZCTreeManager* treeManagerLocal = GetApzcTreeManager();
  return treeManagerLocal
      && treeManagerLocal->DispatchScroll(this, aStartPoint, aEndPoint,
                                          aOverscrollHandoffChain,
                                          aOverscrollHandoffChainIndex);
}

void AsyncPanZoomController::TrackTouch(const MultiTouchInput& aEvent) {
  ScreenPoint prevTouchPoint(mX.GetPos(), mY.GetPos());
  ScreenPoint touchPoint = GetFirstTouchScreenPoint(aEvent);

  ScreenPoint delta(mX.PanDistance(touchPoint.x),
                    mY.PanDistance(touchPoint.y));
  const ScreenPoint panStart = PanStart();
  ToGlobalScreenCoordinates(&delta, panStart);
  HandlePanningUpdate(delta);

  UpdateWithTouchAtDevicePoint(aEvent);

  if (prevTouchPoint != touchPoint) {
    CallDispatchScroll(prevTouchPoint, touchPoint,
        *CurrentTouchBlock()->GetOverscrollHandoffChain(), 0);
  }
}

ScreenPoint AsyncPanZoomController::GetFirstTouchScreenPoint(const MultiTouchInput& aEvent) {
  return ((SingleTouchData&)aEvent.mTouches[0]).mScreenPoint;
}

void AsyncPanZoomController::StartAnimation(AsyncPanZoomAnimation* aAnimation)
{
  ReentrantMonitorAutoEnter lock(mMonitor);
  mAnimation = aAnimation;
  mLastSampleTime = GetFrameTime();
  ScheduleComposite();
}

void AsyncPanZoomController::CancelAnimation() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  APZC_LOG("%p running CancelAnimation in state %d\n", this, mState);
  SetState(NOTHING);
  mAnimation = nullptr;
  
  
  mX.SetVelocity(0);
  mY.SetVelocity(0);
  
  
  
  if (mX.IsOverscrolled() || mY.IsOverscrolled()) {
    ClearOverscroll();
    RequestContentRepaint();
    ScheduleComposite();
    UpdateSharedCompositorFrameMetrics();
  }
}

void AsyncPanZoomController::ClearOverscroll() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  mX.ClearOverscroll();
  mY.ClearOverscroll();
}

void AsyncPanZoomController::SetCompositorParent(CompositorParent* aCompositorParent) {
  mCompositorParent = aCompositorParent;
}

void AsyncPanZoomController::ShareFrameMetricsAcrossProcesses() {
  mSharingFrameMetricsAcrossProcesses = true;
}

void AsyncPanZoomController::ScrollBy(const CSSPoint& aOffset) {
  mFrameMetrics.ScrollBy(aOffset);
}

void AsyncPanZoomController::ScaleWithFocus(float aScale,
                                            const CSSPoint& aFocus) {
  mFrameMetrics.ZoomBy(aScale);
  
  
  
  
  mFrameMetrics.SetScrollOffset((mFrameMetrics.GetScrollOffset() + aFocus) - (aFocus / aScale));
}




static CSSSize
CalculateDisplayPortSize(const CSSSize& aCompositionSize,
                         const CSSPoint& aVelocity)
{
  float xMultiplier = fabsf(aVelocity.x) < gfxPrefs::APZMinSkateSpeed()
                        ? gfxPrefs::APZXStationarySizeMultiplier()
                        : gfxPrefs::APZXSkateSizeMultiplier();
  float yMultiplier = fabsf(aVelocity.y) < gfxPrefs::APZMinSkateSpeed()
                        ? gfxPrefs::APZYStationarySizeMultiplier()
                        : gfxPrefs::APZYSkateSizeMultiplier();

  
  
  
  
  float xSize = std::max(aCompositionSize.width * xMultiplier,
                         aCompositionSize.width + (2 * gfxPrefs::APZDangerZoneX()));
  float ySize = std::max(aCompositionSize.height * yMultiplier,
                         aCompositionSize.height + (2 * gfxPrefs::APZDangerZoneY()));

  return CSSSize(xSize, ySize);
}






static void
RedistributeDisplayPortExcess(CSSSize& aDisplayPortSize,
                              const CSSRect& aScrollableRect)
{
  float xSlack = std::max(0.0f, aDisplayPortSize.width - aScrollableRect.width);
  float ySlack = std::max(0.0f, aDisplayPortSize.height - aScrollableRect.height);

  if (ySlack > 0) {
    
    aDisplayPortSize.height -= ySlack;
    float xExtra = ySlack * aDisplayPortSize.width / aDisplayPortSize.height;
    aDisplayPortSize.width += xExtra;
  } else if (xSlack > 0) {
    
    aDisplayPortSize.width -= xSlack;
    float yExtra = xSlack * aDisplayPortSize.height / aDisplayPortSize.width;
    aDisplayPortSize.height += yExtra;
  }
}


const LayerMargin AsyncPanZoomController::CalculatePendingDisplayPort(
  const FrameMetrics& aFrameMetrics,
  const ScreenPoint& aVelocity,
  double aEstimatedPaintDuration)
{
  CSSSize compositionSize = aFrameMetrics.CalculateBoundedCompositedSizeInCssPixels();
  CSSPoint velocity = aVelocity / aFrameMetrics.GetZoom();
  CSSPoint scrollOffset = aFrameMetrics.GetScrollOffset();
  CSSRect scrollableRect = aFrameMetrics.GetExpandedScrollableRect();

  
  CSSSize displayPortSize = CalculateDisplayPortSize(compositionSize, velocity);

  if (gfxPrefs::APZEnlargeDisplayPortWhenClipped()) {
    RedistributeDisplayPortExcess(displayPortSize, scrollableRect);
  }

  
  
  float estimatedPaintDurationMillis = (float)(aEstimatedPaintDuration * 1000.0);
  float paintFactor = (gfxPrefs::APZUsePaintDuration() ? estimatedPaintDurationMillis : 50.0f);
  CSSRect displayPort = CSSRect(scrollOffset + (velocity * paintFactor * gfxPrefs::APZVelocityBias()),
                                displayPortSize);

  
  displayPort.MoveBy((compositionSize.width - displayPort.width)/2.0f,
                     (compositionSize.height - displayPort.height)/2.0f);

  
  displayPort = displayPort.ForceInside(scrollableRect) - scrollOffset;

  APZC_LOG_FM(aFrameMetrics,
    "Calculated displayport as (%f %f %f %f) from velocity %s paint time %f metrics",
    displayPort.x, displayPort.y, displayPort.width, displayPort.height,
    ToString(aVelocity).c_str(), (float)estimatedPaintDurationMillis);

  CSSMargin cssMargins;
  cssMargins.left = -displayPort.x;
  cssMargins.top = -displayPort.y;
  cssMargins.right = displayPort.width - compositionSize.width - cssMargins.left;
  cssMargins.bottom = displayPort.height - compositionSize.height - cssMargins.top;

  LayerMargin layerMargins = cssMargins * aFrameMetrics.LayersPixelsPerCSSPixel();

  return layerMargins;
}

void AsyncPanZoomController::ScheduleComposite() {
  if (mCompositorParent) {
    mCompositorParent->ScheduleRenderOnCompositorThread();
  }
}

void AsyncPanZoomController::ScheduleCompositeAndMaybeRepaint() {
  ScheduleComposite();

  TimeDuration timePaintDelta = mPaintThrottler.TimeSinceLastRequest(GetFrameTime());
  if (timePaintDelta.ToMilliseconds() > gfxPrefs::APZPanRepaintInterval()) {
    RequestContentRepaint();
  }
}

void AsyncPanZoomController::FlushRepaintForOverscrollHandoff() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  RequestContentRepaint();
  UpdateSharedCompositorFrameMetrics();
}

bool AsyncPanZoomController::SnapBackIfOverscrolled() {
  ReentrantMonitorAutoEnter lock(mMonitor);
  if (IsOverscrolled()) {
    APZC_LOG("%p is overscrolled, starting snap-back\n", this);
    StartSnapBack();
    return true;
  }
  return false;
}

bool AsyncPanZoomController::IsPannable() const {
  ReentrantMonitorAutoEnter lock(mMonitor);
  return mX.CanScroll() || mY.CanScroll();
}

int32_t AsyncPanZoomController::GetLastTouchIdentifier() const {
  nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
  return listener ? listener->GetLastTouchIdentifier() : -1;
}

void AsyncPanZoomController::RequestContentRepaint() {
  RequestContentRepaint(mFrameMetrics);
}

void AsyncPanZoomController::RequestContentRepaint(FrameMetrics& aFrameMetrics) {
  aFrameMetrics.SetDisplayPortMargins(
    CalculatePendingDisplayPort(aFrameMetrics,
                                GetVelocityVector(),
                                mPaintThrottler.AverageDuration().ToSeconds()));
  aFrameMetrics.SetUseDisplayPortMargins();

  
  
  LayerMargin marginDelta = mLastPaintRequestMetrics.GetDisplayPortMargins()
                          - aFrameMetrics.GetDisplayPortMargins();
  if (fabsf(marginDelta.left) < EPSILON &&
      fabsf(marginDelta.top) < EPSILON &&
      fabsf(marginDelta.right) < EPSILON &&
      fabsf(marginDelta.bottom) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.GetScrollOffset().x -
            aFrameMetrics.GetScrollOffset().x) < EPSILON &&
      fabsf(mLastPaintRequestMetrics.GetScrollOffset().y -
            aFrameMetrics.GetScrollOffset().y) < EPSILON &&
      aFrameMetrics.GetZoom() == mLastPaintRequestMetrics.GetZoom() &&
      fabsf(aFrameMetrics.GetViewport().width - mLastPaintRequestMetrics.GetViewport().width) < EPSILON &&
      fabsf(aFrameMetrics.GetViewport().height - mLastPaintRequestMetrics.GetViewport().height) < EPSILON) {
    return;
  }

  SendAsyncScrollEvent();
  mPaintThrottler.PostTask(
    FROM_HERE,
    UniquePtr<CancelableTask>(NewRunnableMethod(this,
                      &AsyncPanZoomController::DispatchRepaintRequest,
                      aFrameMetrics)),
    GetFrameTime());

  aFrameMetrics.SetPresShellId(mLastContentPaintMetrics.GetPresShellId());
  mLastPaintRequestMetrics = aFrameMetrics;
}

 CSSRect
GetDisplayPortRect(const FrameMetrics& aFrameMetrics)
{
  
  
  CSSRect baseRect(aFrameMetrics.GetScrollOffset(),
                   aFrameMetrics.CalculateBoundedCompositedSizeInCssPixels());
  baseRect.Inflate(aFrameMetrics.GetDisplayPortMargins() / aFrameMetrics.LayersPixelsPerCSSPixel());
  return baseRect;
}

void
AsyncPanZoomController::DispatchRepaintRequest(const FrameMetrics& aFrameMetrics) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    APZC_LOG_FM(aFrameMetrics, "%p requesting content repaint", this);
    LogRendertraceRect(GetGuid(), "requested displayport", "yellow", GetDisplayPortRect(aFrameMetrics));

    controller->RequestContentRepaint(aFrameMetrics);
    mLastDispatchedPaintMetrics = aFrameMetrics;
  }
}

void
AsyncPanZoomController::FireAsyncScrollOnTimeout()
{
  if (mCurrentAsyncScrollOffset != mLastAsyncScrollOffset) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    SendAsyncScrollEvent();
  }
  mAsyncScrollTimeoutTask = nullptr;
}

bool AsyncPanZoomController::UpdateAnimation(const TimeStamp& aSampleTime,
                                             Vector<Task*>* aOutDeferredTasks)
{
  AssertOnCompositorThread();

  
  
  
  
  if (mLastSampleTime == aSampleTime) {
    return false;
  }
  TimeDuration sampleTimeDelta = aSampleTime - mLastSampleTime;
  mLastSampleTime = aSampleTime;

  if (mAnimation) {
    bool continueAnimation = mAnimation->Sample(mFrameMetrics, sampleTimeDelta);
    *aOutDeferredTasks = mAnimation->TakeDeferredTasks();
    if (continueAnimation) {
      if (mPaintThrottler.TimeSinceLastRequest(aSampleTime) >
          mAnimation->mRepaintInterval) {
        RequestContentRepaint();
      }
    } else {
      mAnimation = nullptr;
      SetState(NOTHING);
      SendAsyncScrollEvent();
      RequestContentRepaint();
    }
    UpdateSharedCompositorFrameMetrics();
    return true;
  }
  return false;
}

void AsyncPanZoomController::GetOverscrollTransform(Matrix4x4* aTransform) const {
  
  
  

  
  
  const float kStretchFactor = gfxPrefs::APZOverscrollStretchFactor();

  
  
  CSSSize compositionSize(mX.GetCompositionLength(), mY.GetCompositionLength());
  float scaleX = 1 + kStretchFactor * fabsf(mX.GetOverscroll()) / mX.GetCompositionLength();
  float scaleY = 1 + kStretchFactor * fabsf(mY.GetOverscroll()) / mY.GetCompositionLength();

  
  
  
  
  
  CSSPoint translation;
  if (mX.IsOverscrolled() && mX.GetOverscroll() > 0) {
    
    CSSCoord overscrolledCompositionWidth = scaleX * compositionSize.width;
    CSSCoord extraCompositionWidth = overscrolledCompositionWidth - compositionSize.width;
    translation.x = -extraCompositionWidth;
  }
  if (mY.IsOverscrolled() && mY.GetOverscroll() > 0) {
    
    CSSCoord overscrolledCompositionHeight = scaleY * compositionSize.height;
    CSSCoord extraCompositionHeight = overscrolledCompositionHeight - compositionSize.height;
    translation.y = -extraCompositionHeight;
  }

  
  ScreenPoint screenTranslation = translation * mFrameMetrics.GetZoom();
  *aTransform = Matrix4x4().Scale(scaleX, scaleY, 1)
                           .PostTranslate(screenTranslation.x, screenTranslation.y, 0);
}

bool AsyncPanZoomController::AdvanceAnimations(const TimeStamp& aSampleTime)
{
  AssertOnCompositorThread();

  
  
  
  StateChangeNotificationBlocker blocker(this);

  
  
  
  
  
  mAsyncTransformAppliedToContent = false;
  bool requestAnimationFrame = false;
  Vector<Task*> deferredTasks;

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    requestAnimationFrame = UpdateAnimation(aSampleTime, &deferredTasks);

    LogRendertraceRect(GetGuid(), "viewport", "red",
      CSSRect(mFrameMetrics.GetScrollOffset(),
              mFrameMetrics.CalculateCompositedSizeInCssPixels()));

    mCurrentAsyncScrollOffset = mFrameMetrics.GetScrollOffset();
  }

  
  
  
  
  for (uint32_t i = 0; i < deferredTasks.length(); ++i) {
    deferredTasks[i]->Run();
    delete deferredTasks[i];
  }

  
  
  requestAnimationFrame |= (mAnimation != nullptr);

  
  
  if (mAsyncScrollTimeoutTask) {
    mAsyncScrollTimeoutTask->Cancel();
    mAsyncScrollTimeoutTask = nullptr;
  }
  
  
  
  
  
  TimeDuration delta = aSampleTime - mLastAsyncScrollTime;
  if (delta.ToMilliseconds() > gfxPrefs::APZAsyncScrollThrottleTime() &&
      mCurrentAsyncScrollOffset != mLastAsyncScrollOffset) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    mLastAsyncScrollTime = aSampleTime;
    mLastAsyncScrollOffset = mCurrentAsyncScrollOffset;
    SendAsyncScrollEvent();
  } else {
    mAsyncScrollTimeoutTask =
      NewRunnableMethod(this, &AsyncPanZoomController::FireAsyncScrollOnTimeout);
    MessageLoop::current()->PostDelayedTask(FROM_HERE,
                                            mAsyncScrollTimeoutTask,
                                            gfxPrefs::APZAsyncScrollTimeout());
  }

  return requestAnimationFrame;
}

void AsyncPanZoomController::SampleContentTransformForFrame(ViewTransform* aOutTransform,
                                                            ScreenPoint& aScrollOffset,
                                                            Matrix4x4* aOutOverscrollTransform)
{
  ReentrantMonitorAutoEnter lock(mMonitor);

  aScrollOffset = mFrameMetrics.GetScrollOffset() * mFrameMetrics.GetZoom();
  *aOutTransform = GetCurrentAsyncTransform();

  
  
  if (aOutOverscrollTransform && IsOverscrolled()) {
    GetOverscrollTransform(aOutOverscrollTransform);
  }
}

ViewTransform AsyncPanZoomController::GetCurrentAsyncTransform() const {
  ReentrantMonitorAutoEnter lock(mMonitor);

  CSSPoint lastPaintScrollOffset;
  if (mLastContentPaintMetrics.IsScrollable()) {
    lastPaintScrollOffset = mLastContentPaintMetrics.GetScrollOffset();
  }

  CSSPoint currentScrollOffset = mFrameMetrics.GetScrollOffset() +
    mTestAsyncScrollOffset;

  
  
  if (!gfxPrefs::APZAllowCheckerboarding() &&
      !mLastContentPaintMetrics.mDisplayPort.IsEmpty()) {
    CSSSize compositedSize = mLastContentPaintMetrics.CalculateCompositedSizeInCssPixels();
    CSSPoint maxScrollOffset = lastPaintScrollOffset +
      CSSPoint(mLastContentPaintMetrics.mDisplayPort.XMost() - compositedSize.width,
               mLastContentPaintMetrics.mDisplayPort.YMost() - compositedSize.height);
    CSSPoint minScrollOffset = lastPaintScrollOffset + mLastContentPaintMetrics.mDisplayPort.TopLeft();

    if (minScrollOffset.x < maxScrollOffset.x) {
      currentScrollOffset.x = clamped(currentScrollOffset.x, minScrollOffset.x, maxScrollOffset.x);
    }
    if (minScrollOffset.y < maxScrollOffset.y) {
      currentScrollOffset.y = clamped(currentScrollOffset.y, minScrollOffset.y, maxScrollOffset.y);
    }
  }

  ParentLayerToScreenScale scale = mFrameMetrics.GetZoom()
        / mLastContentPaintMetrics.mDevPixelsPerCSSPixel
        / mFrameMetrics.GetParentResolution();
  ScreenPoint translation = (currentScrollOffset - lastPaintScrollOffset)
                          * mFrameMetrics.GetZoom();

  return ViewTransform(scale, -translation);
}

Matrix4x4 AsyncPanZoomController::GetNontransientAsyncTransform() const {
  ReentrantMonitorAutoEnter lock(mMonitor);
  return Matrix4x4().Scale(mLastContentPaintMetrics.mResolution.scale,
                           mLastContentPaintMetrics.mResolution.scale,
                           1.0f);
}

Matrix4x4 AsyncPanZoomController::GetTransformToLastDispatchedPaint() const {
  ReentrantMonitorAutoEnter lock(mMonitor);

  
  
  
  
  
  
  ParentLayerPoint scrollChange =
    (mLastContentPaintMetrics.GetScrollOffset() - mLastDispatchedPaintMetrics.GetScrollOffset())
    * mLastContentPaintMetrics.mDevPixelsPerCSSPixel
    * mLastContentPaintMetrics.GetParentResolution();

  float zoomChange = mLastContentPaintMetrics.GetZoom().scale / mLastDispatchedPaintMetrics.GetZoom().scale;

  return Matrix4x4().Translate(scrollChange.x, scrollChange.y, 0) *
         Matrix4x4().Scale(zoomChange, zoomChange, 1);
}

void AsyncPanZoomController::NotifyLayersUpdated(const FrameMetrics& aLayerMetrics, bool aIsFirstPaint) {
  AssertOnCompositorThread();

  ReentrantMonitorAutoEnter lock(mMonitor);
  bool isDefault = mFrameMetrics.IsDefault();

  mLastContentPaintMetrics = aLayerMetrics;
  UpdateTransformScale();

  mFrameMetrics.mMayHaveTouchListeners = aLayerMetrics.mMayHaveTouchListeners;
  mFrameMetrics.mMayHaveTouchCaret = aLayerMetrics.mMayHaveTouchCaret;
  mFrameMetrics.SetScrollParentId(aLayerMetrics.GetScrollParentId());
  APZC_LOG_FM(aLayerMetrics, "%p got a NotifyLayersUpdated with aIsFirstPaint=%d", this, aIsFirstPaint);

  LogRendertraceRect(GetGuid(), "page", "brown", aLayerMetrics.mScrollableRect);
  LogRendertraceRect(GetGuid(), "painted displayport", "lightgreen",
    aLayerMetrics.mDisplayPort + aLayerMetrics.GetScrollOffset());
  if (!aLayerMetrics.mCriticalDisplayPort.IsEmpty()) {
    LogRendertraceRect(GetGuid(), "painted critical displayport", "darkgreen",
      aLayerMetrics.mCriticalDisplayPort + aLayerMetrics.GetScrollOffset());
  }

  mPaintThrottler.TaskComplete(GetFrameTime());
  bool needContentRepaint = false;
  if (FuzzyEqualsAdditive(aLayerMetrics.mCompositionBounds.width, mFrameMetrics.mCompositionBounds.width) &&
      FuzzyEqualsAdditive(aLayerMetrics.mCompositionBounds.height, mFrameMetrics.mCompositionBounds.height)) {
    
    
    if (mFrameMetrics.GetViewport().width != aLayerMetrics.GetViewport().width ||
        mFrameMetrics.GetViewport().height != aLayerMetrics.GetViewport().height) {
      needContentRepaint = true;
    }
    mFrameMetrics.SetViewport(aLayerMetrics.GetViewport());
  }

  
  
  
  
  bool scrollOffsetUpdated = aLayerMetrics.GetScrollOffsetUpdated()
        && (aLayerMetrics.GetScrollGeneration() != mFrameMetrics.GetScrollGeneration());

  if (aIsFirstPaint || isDefault) {
    
    
    mPaintThrottler.ClearHistory();
    mPaintThrottler.SetMaxDurations(gfxPrefs::APZNumPaintDurationSamples());

    CancelAnimation();

    mFrameMetrics = aLayerMetrics;
    mLastDispatchedPaintMetrics = aLayerMetrics;
    ShareCompositorFrameMetrics();

    if (mFrameMetrics.GetDisplayPortMargins() != LayerMargin()) {
      
      
      
      
      APZC_LOG("%p detected non-empty margins which probably need updating\n", this);
      needContentRepaint = true;
    }
  } else {
    bool smoothScrollRequested = aLayerMetrics.GetDoSmoothScroll()
         && (aLayerMetrics.GetScrollGeneration() != mFrameMetrics.GetScrollGeneration());

    
    
    

    if (FuzzyEqualsAdditive(mFrameMetrics.mCompositionBounds.width, aLayerMetrics.mCompositionBounds.width) &&
        mFrameMetrics.mDevPixelsPerCSSPixel == aLayerMetrics.mDevPixelsPerCSSPixel) {
      float parentResolutionChange = aLayerMetrics.GetParentResolution().scale
                                   / mFrameMetrics.GetParentResolution().scale;
      mFrameMetrics.ZoomBy(parentResolutionChange);
    } else {
      
      
      mFrameMetrics.SetZoom(aLayerMetrics.GetZoom());
      mFrameMetrics.mDevPixelsPerCSSPixel.scale = aLayerMetrics.mDevPixelsPerCSSPixel.scale;
    }
    if (!mFrameMetrics.mScrollableRect.IsEqualEdges(aLayerMetrics.mScrollableRect)) {
      mFrameMetrics.mScrollableRect = aLayerMetrics.mScrollableRect;
      needContentRepaint = true;
    }
    mFrameMetrics.mCompositionBounds = aLayerMetrics.mCompositionBounds;
    mFrameMetrics.SetRootCompositionSize(aLayerMetrics.GetRootCompositionSize());
    mFrameMetrics.mResolution = aLayerMetrics.mResolution;
    mFrameMetrics.mCumulativeResolution = aLayerMetrics.mCumulativeResolution;
    mFrameMetrics.SetHasScrollgrab(aLayerMetrics.GetHasScrollgrab());

    if (scrollOffsetUpdated) {
      APZC_LOG("%p updating scroll offset from %s to %s\n", this,
        ToString(mFrameMetrics.GetScrollOffset()).c_str(),
        ToString(aLayerMetrics.GetScrollOffset()).c_str());

      mFrameMetrics.CopyScrollInfoFrom(aLayerMetrics);

      
      
      
      CancelAnimation();

      
      
      
      
      
      mLastDispatchedPaintMetrics = aLayerMetrics;
    }

    if (smoothScrollRequested) {
      
      
      

      APZC_LOG("%p smooth scrolling from %s to %s\n", this,
        Stringify(mFrameMetrics.GetScrollOffset()).c_str(),
        Stringify(aLayerMetrics.GetSmoothScrollOffset()).c_str());

      mFrameMetrics.CopySmoothScrollInfoFrom(aLayerMetrics);
      CancelAnimation();
      mLastDispatchedPaintMetrics = aLayerMetrics;
      StartSmoothScroll();

      scrollOffsetUpdated = true; 
    }
  }

  if (scrollOffsetUpdated) {
    
    
    
    
    nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
    if (controller) {
      APZC_LOG("%p sending scroll update acknowledgement with gen %lu\n", this, aLayerMetrics.GetScrollGeneration());
      controller->AcknowledgeScrollUpdate(aLayerMetrics.GetScrollId(),
                                          aLayerMetrics.GetScrollGeneration());
    }
  }

  if (needContentRepaint) {
    RequestContentRepaint();
  }
  UpdateSharedCompositorFrameMetrics();
}

const FrameMetrics& AsyncPanZoomController::GetFrameMetrics() const {
  mMonitor.AssertCurrentThreadIn();
  return mFrameMetrics;
}

APZCTreeManager* AsyncPanZoomController::GetApzcTreeManager() const {
  mMonitor.AssertNotCurrentThreadIn();
  return mTreeManager;
}

void AsyncPanZoomController::ZoomToRect(CSSRect aRect) {
  if (!aRect.IsFinite()) {
    NS_WARNING("ZoomToRect got called with a non-finite rect; ignoring...\n");
    return;
  }

  SetState(ANIMATING_ZOOM);

  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    ParentLayerRect compositionBounds = mFrameMetrics.mCompositionBounds;
    CSSRect cssPageRect = mFrameMetrics.mScrollableRect;
    CSSPoint scrollOffset = mFrameMetrics.GetScrollOffset();
    CSSToParentLayerScale currentZoom = mFrameMetrics.GetZoomToParent();
    CSSToParentLayerScale targetZoom;

    
    
    
    
    
    CSSToParentLayerScale localMinZoom(std::max((mZoomConstraints.mMinZoom * mFrameMetrics.mTransformScale).scale,
                                       std::max(compositionBounds.width / cssPageRect.width,
                                                compositionBounds.height / cssPageRect.height)));
    CSSToParentLayerScale localMaxZoom = mZoomConstraints.mMaxZoom * mFrameMetrics.mTransformScale;

    if (!aRect.IsEmpty()) {
      
      aRect = aRect.Intersect(cssPageRect);
      targetZoom = CSSToParentLayerScale(std::min(compositionBounds.width / aRect.width,
                                                  compositionBounds.height / aRect.height));
    }
    
    
    
    
    if (aRect.IsEmpty() ||
        (currentZoom == localMaxZoom && targetZoom >= localMaxZoom) ||
        (currentZoom == localMinZoom && targetZoom <= localMinZoom)) {
      CSSSize compositedSize = mFrameMetrics.CalculateCompositedSizeInCssPixels();
      float y = scrollOffset.y;
      float newHeight =
        cssPageRect.width * (compositedSize.height / compositedSize.width);
      float dh = compositedSize.height - newHeight;

      aRect = CSSRect(0.0f,
                      y + dh/2,
                      cssPageRect.width,
                      newHeight);
      aRect = aRect.Intersect(cssPageRect);
      targetZoom = CSSToParentLayerScale(std::min(compositionBounds.width / aRect.width,
                                                  compositionBounds.height / aRect.height));
    }

    targetZoom.scale = clamped(targetZoom.scale, localMinZoom.scale, localMaxZoom.scale);
    FrameMetrics endZoomToMetrics = mFrameMetrics;
    endZoomToMetrics.SetZoom(targetZoom / mFrameMetrics.mTransformScale);

    
    CSSSize sizeAfterZoom = endZoomToMetrics.CalculateCompositedSizeInCssPixels();

    
    
    if (aRect.y + sizeAfterZoom.height > cssPageRect.height) {
      aRect.y = cssPageRect.height - sizeAfterZoom.height;
      aRect.y = aRect.y > 0 ? aRect.y : 0;
    }
    if (aRect.x + sizeAfterZoom.width > cssPageRect.width) {
      aRect.x = cssPageRect.width - sizeAfterZoom.width;
      aRect.x = aRect.x > 0 ? aRect.x : 0;
    }

    endZoomToMetrics.SetScrollOffset(aRect.TopLeft());
    endZoomToMetrics.SetDisplayPortMargins(
      CalculatePendingDisplayPort(endZoomToMetrics,
                                  ScreenPoint(0,0),
                                  0));
    endZoomToMetrics.SetUseDisplayPortMargins();

    StartAnimation(new ZoomAnimation(
        mFrameMetrics.GetScrollOffset(),
        mFrameMetrics.GetZoom(),
        endZoomToMetrics.GetScrollOffset(),
        endZoomToMetrics.GetZoom()));

    
    
    RequestContentRepaint(endZoomToMetrics);
  }
}

void
AsyncPanZoomController::ScheduleContentResponseTimeout() {
  APZC_LOG("%p scheduling content response timeout\n", this);
  PostDelayedTask(
    NewRunnableMethod(this, &AsyncPanZoomController::ContentResponseTimeout),
    gfxPrefs::APZContentResponseTimeout());
}

void
AsyncPanZoomController::ContentResponseTimeout() {
  AssertOnControllerThread();

  mTouchBlockBalance++;
  APZC_LOG("%p got a content response timeout; balance %d\n", this, mTouchBlockBalance);
  if (mTouchBlockBalance > 0) {
    
    
    bool found = false;
    for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
      if (mTouchBlockQueue[i]->TimeoutContentResponse()) {
        found = true;
        break;
      }
    }
    if (found) {
      ProcessPendingInputBlocks();
    } else {
      NS_WARNING("APZC received more ContentResponseTimeout calls than it has unprocessed touch blocks\n");
    }
  }
}

void
AsyncPanZoomController::ContentReceivedTouch(bool aPreventDefault) {
  AssertOnControllerThread();

  mTouchBlockBalance--;
  APZC_LOG("%p got a content response; balance %d\n", this, mTouchBlockBalance);
  if (mTouchBlockBalance < 0) {
    
    
    bool found = false;
    for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
      if (mTouchBlockQueue[i]->SetContentResponse(aPreventDefault)) {
        found = true;
        break;
      }
    }
    if (found) {
      ProcessPendingInputBlocks();
    } else {
      NS_WARNING("APZC received more ContentReceivedTouch calls than it has unprocessed touch blocks\n");
    }
  }
}

void
AsyncPanZoomController::SetAllowedTouchBehavior(const nsTArray<TouchBehaviorFlags>& aBehaviors) {
  AssertOnControllerThread();

  bool found = false;
  for (size_t i = 0; i < mTouchBlockQueue.Length(); i++) {
    if (mTouchBlockQueue[i]->SetAllowedTouchBehaviors(aBehaviors)) {
      found = true;
      break;
    }
  }
  if (found) {
    ProcessPendingInputBlocks();
  } else {
    NS_WARNING("APZC received more SetAllowedTouchBehavior calls than it has unprocessed touch blocks\n");
  }
}

void
AsyncPanZoomController::ProcessPendingInputBlocks() {
  AssertOnControllerThread();

  while (true) {
    TouchBlockState* curBlock = CurrentTouchBlock();
    if (!curBlock->IsReadyForHandling()) {
      break;
    }

    APZC_LOG("%p processing input block %p; preventDefault %d\n",
        this, curBlock, curBlock->IsDefaultPrevented());
    if (curBlock->IsDefaultPrevented()) {
      SetState(NOTHING);
      curBlock->DropEvents();
      
      nsRefPtr<GestureEventListener> listener = GetGestureEventListener();
      if (listener) {
        MultiTouchInput cancel(MultiTouchInput::MULTITOUCH_CANCEL, 0, TimeStamp::Now(), 0);
        listener->HandleInputEvent(cancel);
      }
    } else {
      while (curBlock->HasEvents()) {
        HandleInputEvent(curBlock->RemoveFirstEvent());
      }
    }
    MOZ_ASSERT(!curBlock->HasEvents());

    if (mTouchBlockQueue.Length() == 1) {
      
      
      
      
      break;
    }

    
    
    APZC_LOG("%p discarding depleted touch block %p\n", this, curBlock);
    mTouchBlockQueue.RemoveElementAt(0);
  }
}

TouchBlockState*
AsyncPanZoomController::StartNewTouchBlock(bool aCopyAllowedTouchBehaviorFromCurrent)
{
  TouchBlockState* newBlock = new TouchBlockState(BuildOverscrollHandoffChain());
  if (gfxPrefs::TouchActionEnabled() && aCopyAllowedTouchBehaviorFromCurrent) {
    newBlock->CopyAllowedTouchBehaviorsFrom(*CurrentTouchBlock());
  }

  
  
  while (!mTouchBlockQueue.IsEmpty()) {
    if (mTouchBlockQueue[0]->IsReadyForHandling() && !mTouchBlockQueue[0]->HasEvents()) {
      APZC_LOG("%p discarding depleted touch block %p\n", this, mTouchBlockQueue[0].get());
      mTouchBlockQueue.RemoveElementAt(0);
    } else {
      break;
    }
  }

  
  mTouchBlockQueue.AppendElement(newBlock);
  return newBlock;
}

TouchBlockState*
AsyncPanZoomController::CurrentTouchBlock()
{
  AssertOnControllerThread();

  MOZ_ASSERT(!mTouchBlockQueue.IsEmpty());
  return mTouchBlockQueue[0].get();
}

bool
AsyncPanZoomController::HasReadyTouchBlock()
{
  return !mTouchBlockQueue.IsEmpty() && mTouchBlockQueue[0]->IsReadyForHandling();
}

AsyncPanZoomController::TouchBehaviorFlags
AsyncPanZoomController::GetAllowedTouchBehavior(ScreenIntPoint& aPoint) {
  
  
  
  return AllowedTouchBehavior::UNKNOWN;
}

void AsyncPanZoomController::SetState(PanZoomState aNewState)
{
  PanZoomState oldState;

  
  {
    ReentrantMonitorAutoEnter lock(mMonitor);
    oldState = mState;
    mState = aNewState;
  }

  DispatchStateChangeNotification(oldState, aNewState);
}

void AsyncPanZoomController::DispatchStateChangeNotification(PanZoomState aOldState,
                                                             PanZoomState aNewState)
{
  { 
    ReentrantMonitorAutoEnter lock(mMonitor);
    if (mNotificationBlockers > 0) {
      return;
    }
  }

  if (nsRefPtr<GeckoContentController> controller = GetGeckoContentController()) {
    if (!IsTransformingState(aOldState) && IsTransformingState(aNewState)) {
      controller->NotifyAPZStateChange(
          GetGuid(), APZStateChange::TransformBegin);
    } else if (IsTransformingState(aOldState) && !IsTransformingState(aNewState)) {
      controller->NotifyAPZStateChange(
          GetGuid(), APZStateChange::TransformEnd);
    }
  }
}

bool AsyncPanZoomController::IsTransformingState(PanZoomState aState) {
  return !(aState == NOTHING || aState == TOUCHING);
}

bool AsyncPanZoomController::IsInPanningState() const {
  return (mState == PANNING || mState == PANNING_LOCKED_X || mState == PANNING_LOCKED_Y);
}

void AsyncPanZoomController::UpdateZoomConstraints(const ZoomConstraints& aConstraints) {
  APZC_LOG("%p updating zoom constraints to %d %d %f %f\n", this, aConstraints.mAllowZoom,
    aConstraints.mAllowDoubleTapZoom, aConstraints.mMinZoom.scale, aConstraints.mMaxZoom.scale);
  if (IsNaN(aConstraints.mMinZoom.scale) || IsNaN(aConstraints.mMaxZoom.scale)) {
    NS_WARNING("APZC received zoom constraints with NaN values; dropping...\n");
    return;
  }
  
  mZoomConstraints.mAllowZoom = aConstraints.mAllowZoom;
  mZoomConstraints.mAllowDoubleTapZoom = aConstraints.mAllowDoubleTapZoom;
  mZoomConstraints.mMinZoom = (MIN_ZOOM > aConstraints.mMinZoom ? MIN_ZOOM : aConstraints.mMinZoom);
  mZoomConstraints.mMaxZoom = (MAX_ZOOM > aConstraints.mMaxZoom ? aConstraints.mMaxZoom : MAX_ZOOM);
  if (mZoomConstraints.mMaxZoom < mZoomConstraints.mMinZoom) {
    mZoomConstraints.mMaxZoom = mZoomConstraints.mMinZoom;
  }
}

ZoomConstraints
AsyncPanZoomController::GetZoomConstraints() const
{
  return mZoomConstraints;
}


void AsyncPanZoomController::PostDelayedTask(Task* aTask, int aDelayMs) {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (controller) {
    controller->PostDelayedTask(aTask, aDelayMs);
  }
}

void AsyncPanZoomController::SendAsyncScrollEvent() {
  nsRefPtr<GeckoContentController> controller = GetGeckoContentController();
  if (!controller) {
    return;
  }

  bool isRoot;
  CSSRect contentRect;
  CSSSize scrollableSize;
  {
    ReentrantMonitorAutoEnter lock(mMonitor);

    isRoot = mFrameMetrics.GetIsRoot();
    scrollableSize = mFrameMetrics.mScrollableRect.Size();
    contentRect = mFrameMetrics.CalculateCompositedRectInCssPixels();
    contentRect.MoveTo(mCurrentAsyncScrollOffset);
  }

  controller->SendAsyncScrollDOMEvent(isRoot, contentRect, scrollableSize);
}

bool AsyncPanZoomController::Matches(const ScrollableLayerGuid& aGuid)
{
  return aGuid == GetGuid();
}

void AsyncPanZoomController::GetGuid(ScrollableLayerGuid* aGuidOut) const
{
  if (aGuidOut) {
    *aGuidOut = GetGuid();
  }
}

ScrollableLayerGuid AsyncPanZoomController::GetGuid() const
{
  return ScrollableLayerGuid(mLayersId, mFrameMetrics);
}

void AsyncPanZoomController::UpdateSharedCompositorFrameMetrics()
{
  mMonitor.AssertCurrentThreadIn();

  FrameMetrics* frame = mSharedFrameMetricsBuffer ?
      static_cast<FrameMetrics*>(mSharedFrameMetricsBuffer->memory()) : nullptr;

  if (frame && mSharedLock && gfxPrefs::UseProgressiveTilePainting()) {
    mSharedLock->Lock();
    *frame = mFrameMetrics.MakePODObject();
    mSharedLock->Unlock();
  }
}

void AsyncPanZoomController::ShareCompositorFrameMetrics() {

  PCompositorParent* compositor = GetSharedFrameMetricsCompositor();

  
  
  
  if (!mSharedFrameMetricsBuffer && compositor && gfxPrefs::UseProgressiveTilePainting()) {

    
    mSharedFrameMetricsBuffer = new ipc::SharedMemoryBasic;
    FrameMetrics* frame = nullptr;
    mSharedFrameMetricsBuffer->Create(sizeof(FrameMetrics));
    mSharedFrameMetricsBuffer->Map(sizeof(FrameMetrics));
    frame = static_cast<FrameMetrics*>(mSharedFrameMetricsBuffer->memory());

    if (frame) {

      { 
        ReentrantMonitorAutoEnter lock(mMonitor);
        *frame = mFrameMetrics;
      }

      
      base::ProcessHandle processHandle = compositor->OtherProcess();
      ipc::SharedMemoryBasic::Handle mem = ipc::SharedMemoryBasic::NULLHandle();

      
      mSharedFrameMetricsBuffer->ShareToProcess(processHandle, &mem);

      
      mSharedLock = new CrossProcessMutex("AsyncPanZoomControlLock");
      CrossProcessMutexHandle handle = mSharedLock->ShareToProcess(processHandle);

      
      
      
      if (!compositor->SendSharedCompositorFrameMetrics(mem, handle, mAPZCId)) {
        APZC_LOG("%p failed to share FrameMetrics with content process.", this);
      }
    }
  }
}

ParentLayerPoint AsyncPanZoomController::ToParentLayerCoords(const ScreenPoint& aPoint)
{
  
  
  return ParentLayerPoint(aPoint.x, aPoint.y);
}

void AsyncPanZoomController::UpdateTransformScale()
{
  
  
  mFrameMetrics.mTransformScale.scale = 1;
}

}
}
