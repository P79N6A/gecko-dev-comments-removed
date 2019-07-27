




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/Attributes.h"
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/LayerMetricsWrapper.h"
#include "mozilla/UniquePtr.h"
#include "base/task.h"
#include "Layers.h"
#include "TestLayers.h"
#include "gfxPrefs.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::AtLeast;
using ::testing::AtMost;
using ::testing::MockFunction;
using ::testing::InSequence;

class Task;

template<class T>
class ScopedGfxPref {
public:
  ScopedGfxPref(T (*aGetPrefFunc)(void), void (*aSetPrefFunc)(T), T aVal)
    : mSetPrefFunc(aSetPrefFunc)
  {
    mOldVal = aGetPrefFunc();
    aSetPrefFunc(aVal);
  }

  ~ScopedGfxPref() {
    mSetPrefFunc(mOldVal);
  }

private:
  void (*mSetPrefFunc)(T);
  T mOldVal;
};

#define SCOPED_GFX_PREF(prefBase, prefType, prefValue) \
  ScopedGfxPref<prefType> pref_##prefBase( \
    &(gfxPrefs::prefBase), \
    &(gfxPrefs::Set##prefBase), \
    prefValue)

class MockContentController : public GeckoContentController {
public:
  MOCK_METHOD1(RequestContentRepaint, void(const FrameMetrics&));
  MOCK_METHOD2(AcknowledgeScrollUpdate, void(const FrameMetrics::ViewID&, const uint32_t& aScrollGeneration));
  MOCK_METHOD3(HandleDoubleTap, void(const CSSPoint&, int32_t, const ScrollableLayerGuid&));
  MOCK_METHOD3(HandleSingleTap, void(const CSSPoint&, int32_t, const ScrollableLayerGuid&));
  MOCK_METHOD3(HandleLongTap, void(const CSSPoint&, int32_t, const ScrollableLayerGuid&));
  MOCK_METHOD3(HandleLongTapUp, void(const CSSPoint&, int32_t, const ScrollableLayerGuid&));
  MOCK_METHOD3(SendAsyncScrollDOMEvent, void(bool aIsRoot, const CSSRect &aContentRect, const CSSSize &aScrollableSize));
  MOCK_METHOD2(PostDelayedTask, void(Task* aTask, int aDelayMs));
};

class MockContentControllerDelayed : public MockContentController {
public:
  MockContentControllerDelayed()
  {
  }

  void PostDelayedTask(Task* aTask, int aDelayMs) {
    mTaskQueue.AppendElement(aTask);
  }

  void CheckHasDelayedTask() {
    EXPECT_TRUE(mTaskQueue.Length() > 0);
  }

  void ClearDelayedTask() {
    mTaskQueue.RemoveElementAt(0);
  }

  void DestroyOldestTask() {
    delete mTaskQueue[0];
    mTaskQueue.RemoveElementAt(0);
  }

  
  
  
  
  void RunDelayedTask() {
    mTaskQueue[0]->Run();
    delete mTaskQueue[0];
    mTaskQueue.RemoveElementAt(0);
  }

  
  
  
  
  
  int RunThroughDelayedTasks() {
    int numTasks = mTaskQueue.Length();
    for (int i = 0; i < numTasks; i++) {
      RunDelayedTask();
    }
    return numTasks;
  }

private:
  nsTArray<Task*> mTaskQueue;
};

class TestAsyncPanZoomController : public AsyncPanZoomController {
public:
  TestAsyncPanZoomController(uint64_t aLayersId, MockContentController* aMcc,
                             APZCTreeManager* aTreeManager = nullptr,
                             GestureBehavior aBehavior = DEFAULT_GESTURES)
    : AsyncPanZoomController(aLayersId, aTreeManager, aMcc, aBehavior)
  {}

  void SetFrameMetrics(const FrameMetrics& metrics) {
    ReentrantMonitorAutoEnter lock(mMonitor);
    mFrameMetrics = metrics;
  }

  FrameMetrics& GetFrameMetrics() {
    ReentrantMonitorAutoEnter lock(mMonitor);
    return mFrameMetrics;
  }

  const FrameMetrics& GetFrameMetrics() const {
    ReentrantMonitorAutoEnter lock(mMonitor);
    return mFrameMetrics;
  }

  void AssertStateIsReset() const {
    ReentrantMonitorAutoEnter lock(mMonitor);
    EXPECT_EQ(NOTHING, mState);
  }

  void AssertStateIsFling() const {
    ReentrantMonitorAutoEnter lock(mMonitor);
    EXPECT_EQ(FLING, mState);
  }

  void AdvanceAnimationsUntilEnd(TimeStamp& aSampleTime,
                                 const TimeDuration& aIncrement = TimeDuration::FromMilliseconds(10)) {
    while (AdvanceAnimations(aSampleTime)) {
      aSampleTime += aIncrement;
    }
  }

  bool SampleContentTransformForFrame(const TimeStamp& aSampleTime,
                                      ViewTransform* aOutTransform,
                                      ScreenPoint& aScrollOffset) {
    Matrix4x4 aOverscrollTransform;  
    bool ret = AdvanceAnimations(aSampleTime);
    AsyncPanZoomController::SampleContentTransformForFrame(
      aOutTransform, aScrollOffset, &aOverscrollTransform);
    return ret;
  }
};

class TestAPZCTreeManager : public APZCTreeManager {
};

static FrameMetrics
TestFrameMetrics()
{
  FrameMetrics fm;

  fm.mDisplayPort = CSSRect(0, 0, 10, 10);
  fm.mCompositionBounds = ParentLayerRect(0, 0, 10, 10);
  fm.mCriticalDisplayPort = CSSRect(0, 0, 10, 10);
  fm.mScrollableRect = CSSRect(0, 0, 100, 100);

  return fm;
}

class APZCBasicTester : public ::testing::Test {
public:
  explicit APZCBasicTester(AsyncPanZoomController::GestureBehavior aGestureBehavior = AsyncPanZoomController::DEFAULT_GESTURES)
    : mGestureBehavior(aGestureBehavior)
  {
  }

protected:
  virtual void SetUp()
  {
    gfxPrefs::GetSingleton();
    AsyncPanZoomController::SetThreadAssertionsEnabled(false);

    testStartTime = TimeStamp::Now();
    AsyncPanZoomController::SetFrameTime(testStartTime);

    mcc = new NiceMock<MockContentControllerDelayed>();
    tm = new TestAPZCTreeManager();
    apzc = new TestAsyncPanZoomController(0, mcc, tm, mGestureBehavior);
    apzc->SetFrameMetrics(TestFrameMetrics());
  }

  virtual void TearDown()
  {
    apzc->Destroy();
  }

  void SetMayHaveTouchListeners()
  {
    apzc->GetFrameMetrics().mMayHaveTouchListeners = true;
  }

  void MakeApzcZoomable()
  {
    apzc->UpdateZoomConstraints(ZoomConstraints(true, true, CSSToScreenScale(0.25f), CSSToScreenScale(4.0f)));
  }

  void MakeApzcUnzoomable()
  {
    apzc->UpdateZoomConstraints(ZoomConstraints(false, false, CSSToScreenScale(1.0f), CSSToScreenScale(1.0f)));
  }

  AsyncPanZoomController::GestureBehavior mGestureBehavior;
  TimeStamp testStartTime;
  nsRefPtr<MockContentControllerDelayed> mcc;
  nsRefPtr<TestAPZCTreeManager> tm;
  nsRefPtr<TestAsyncPanZoomController> apzc;
};

class APZCGestureDetectorTester : public APZCBasicTester {
public:
  APZCGestureDetectorTester()
    : APZCBasicTester(AsyncPanZoomController::USE_GESTURE_DETECTOR)
  {
  }
};

static nsEventStatus
ApzcDown(AsyncPanZoomController* apzc, int aX, int aY, int aTime)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0));
  return apzc->ReceiveInputEvent(mti);
}

static nsEventStatus
ApzcUp(AsyncPanZoomController* apzc, int aX, int aY, int aTime)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0));
  return apzc->ReceiveInputEvent(mti);
}

static void
ApzcTap(AsyncPanZoomController* aApzc, int aX, int aY, int& aTime, int aTapLength,
        nsEventStatus (*aOutEventStatuses)[2] = nullptr)
{
  nsEventStatus status = ApzcDown(aApzc, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }
  aTime += aTapLength;
  status = ApzcUp(aApzc, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }
}

static void
ApzcTapAndCheckStatus(AsyncPanZoomController* aApzc, int aX, int aY, int& aTime, int aTapLength)
{
  nsEventStatus statuses[2];
  ApzcTap(aApzc, aX, aY, aTime, aTapLength, &statuses);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[0]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[1]);
}

static void
ApzcPan(AsyncPanZoomController* aApzc,
        int& aTime,
        int aTouchStartY,
        int aTouchEndY,
        bool aKeepFingerDown = false,
        nsTArray<uint32_t>* aAllowedTouchBehaviors = nullptr,
        nsEventStatus (*aOutEventStatuses)[4] = nullptr)
{
  const int TIME_BETWEEN_TOUCH_EVENT = 100;
  const int OVERCOME_TOUCH_TOLERANCE = 100;

  
  nsEventStatus status = ApzcDown(aApzc, 10, aTouchStartY + OVERCOME_TOUCH_TOLERANCE, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  
  if (gfxPrefs::TouchActionEnabled() && aAllowedTouchBehaviors) {
    aApzc->SetAllowedTouchBehavior(*aAllowedTouchBehaviors);
  }

  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchStartY), ScreenSize(0, 0), 0, 0));
  status = aApzc->ReceiveInputEvent(mti);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchEndY), ScreenSize(0, 0), 0, 0));
  status = aApzc->ReceiveInputEvent(mti);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  if (!aKeepFingerDown) {
    status = ApzcUp(aApzc, 10, aTouchEndY, aTime);
  } else {
    status = nsEventStatus_eIgnore;
  }
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;
}





static void
ApzcPanAndCheckStatus(AsyncPanZoomController* aApzc,
                      int& aTime,
                      int aTouchStartY,
                      int aTouchEndY,
                      bool aExpectConsumed,
                      nsTArray<uint32_t>* aAllowedTouchBehaviors)
{
  nsEventStatus statuses[4]; 
  ApzcPan(aApzc, aTime, aTouchStartY, aTouchEndY, false, aAllowedTouchBehaviors, &statuses);

  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[0]);

  nsEventStatus touchMoveStatus;
  if (aExpectConsumed) {
    touchMoveStatus = nsEventStatus_eConsumeDoDefault;
  } else {
    touchMoveStatus = nsEventStatus_eIgnore;
  }
  EXPECT_EQ(touchMoveStatus, statuses[1]);
  EXPECT_EQ(touchMoveStatus, statuses[2]);
}

static void
ApzcPanNoFling(AsyncPanZoomController* aApzc,
               int& aTime,
               int aTouchStartY,
               int aTouchEndY)
{
  ApzcPan(aApzc, aTime, aTouchStartY, aTouchEndY);
  aApzc->CancelAnimation();
}

static void
ApzcPinchWithPinchInput(AsyncPanZoomController* aApzc,
                        int aFocusX, int aFocusY, float aScale,
                        nsEventStatus (*aOutEventStatuses)[3] = nullptr)
{
  nsEventStatus actualStatus = aApzc->HandleGestureEvent(
    PinchGestureInput(PinchGestureInput::PINCHGESTURE_START,
                      0, TimeStamp(), ScreenPoint(aFocusX, aFocusY),
                      10.0, 10.0, 0));
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = actualStatus;
  }
  actualStatus = aApzc->HandleGestureEvent(
    PinchGestureInput(PinchGestureInput::PINCHGESTURE_SCALE,
                      0, TimeStamp(), ScreenPoint(aFocusX, aFocusY),
                      10.0 * aScale, 10.0, 0));
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = actualStatus;
  }
  actualStatus = aApzc->HandleGestureEvent(
    PinchGestureInput(PinchGestureInput::PINCHGESTURE_END,
                      0, TimeStamp(), ScreenPoint(aFocusX, aFocusY),
                      
                      
                      -1.0, -1.0, 0));
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = actualStatus;
  }
}

static void
ApzcPinchWithPinchInputAndCheckStatus(AsyncPanZoomController* aApzc,
                                      int aFocusX, int aFocusY, float aScale,
                                      bool aShouldTriggerPinch)
{
  nsEventStatus statuses[3];  
  ApzcPinchWithPinchInput(aApzc, aFocusX, aFocusY, aScale, &statuses);

  nsEventStatus expectedStatus = aShouldTriggerPinch
      ? nsEventStatus_eConsumeNoDefault
      : nsEventStatus_eIgnore;
  EXPECT_EQ(expectedStatus, statuses[0]);
  EXPECT_EQ(expectedStatus, statuses[1]);
}

static void
ApzcPinchWithTouchInput(AsyncPanZoomController* aApzc,
                        int aFocusX, int aFocusY, float aScale,
                        int& inputId,
                        nsTArray<uint32_t>* aAllowedTouchBehaviors = nullptr,
                        nsEventStatus (*aOutEventStatuses)[4] = nullptr)
{
  
  
  float pinchLength = 100.0;
  float pinchLengthScaled = pinchLength * aScale;

  MultiTouchInput mtiStart = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, 0, TimeStamp(), 0);
  mtiStart.mTouches.AppendElement(SingleTouchData(inputId, ScreenIntPoint(aFocusX, aFocusY), ScreenSize(0, 0), 0, 0));
  mtiStart.mTouches.AppendElement(SingleTouchData(inputId + 1, ScreenIntPoint(aFocusX, aFocusY), ScreenSize(0, 0), 0, 0));
  nsEventStatus status = aApzc->ReceiveInputEvent(mtiStart);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }

  if (gfxPrefs::TouchActionEnabled() && aAllowedTouchBehaviors) {
    aApzc->SetAllowedTouchBehavior(*aAllowedTouchBehaviors);
  }

  MultiTouchInput mtiMove1 = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, 0, TimeStamp(), 0);
  mtiMove1.mTouches.AppendElement(SingleTouchData(inputId, ScreenIntPoint(aFocusX - pinchLength, aFocusY), ScreenSize(0, 0), 0, 0));
  mtiMove1.mTouches.AppendElement(SingleTouchData(inputId + 1, ScreenIntPoint(aFocusX + pinchLength, aFocusY), ScreenSize(0, 0), 0, 0));
  status = aApzc->ReceiveInputEvent(mtiMove1);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }

  MultiTouchInput mtiMove2 = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, 0, TimeStamp(), 0);
  mtiMove2.mTouches.AppendElement(SingleTouchData(inputId, ScreenIntPoint(aFocusX - pinchLengthScaled, aFocusY), ScreenSize(0, 0), 0, 0));
  mtiMove2.mTouches.AppendElement(SingleTouchData(inputId + 1, ScreenIntPoint(aFocusX + pinchLengthScaled, aFocusY), ScreenSize(0, 0), 0, 0));
  status = aApzc->ReceiveInputEvent(mtiMove2);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = status;
  }

  MultiTouchInput mtiEnd = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, 0, TimeStamp(), 0);
  mtiEnd.mTouches.AppendElement(SingleTouchData(inputId, ScreenIntPoint(aFocusX - pinchLengthScaled, aFocusY), ScreenSize(0, 0), 0, 0));
  mtiEnd.mTouches.AppendElement(SingleTouchData(inputId + 1, ScreenIntPoint(aFocusX + pinchLengthScaled, aFocusY), ScreenSize(0, 0), 0, 0));
  status = aApzc->ReceiveInputEvent(mtiEnd);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }

  inputId += 2;
}

static void
ApzcPinchWithTouchInputAndCheckStatus(AsyncPanZoomController* aApzc,
                                      int aFocusX, int aFocusY, float aScale,
                                      int& inputId, bool aShouldTriggerPinch,
                                      nsTArray<uint32_t>* aAllowedTouchBehaviors)
{
  nsEventStatus statuses[4];  
  ApzcPinchWithTouchInput(aApzc, aFocusX, aFocusY, aScale, inputId, aAllowedTouchBehaviors, &statuses);

  nsEventStatus expectedMoveStatus = aShouldTriggerPinch
      ? nsEventStatus_eConsumeDoDefault
      : nsEventStatus_eIgnore;
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[0]);
  EXPECT_EQ(expectedMoveStatus, statuses[1]);
  EXPECT_EQ(expectedMoveStatus, statuses[2]);
}

class APZCPinchTester : public APZCBasicTester {
public:
  explicit APZCPinchTester(AsyncPanZoomController::GestureBehavior aGestureBehavior = AsyncPanZoomController::DEFAULT_GESTURES)
    : APZCBasicTester(aGestureBehavior)
  {
  }

protected:
  FrameMetrics GetPinchableFrameMetrics()
  {
    FrameMetrics fm;
    fm.mCompositionBounds = ParentLayerRect(200, 200, 100, 200);
    fm.mScrollableRect = CSSRect(0, 0, 980, 1000);
    fm.SetScrollOffset(CSSPoint(300, 300));
    fm.SetZoom(CSSToScreenScale(2.0));
    
    return fm;
  }

  void DoPinchTest(bool aShouldTriggerPinch,
                   nsTArray<uint32_t> *aAllowedTouchBehaviors = nullptr)
  {
    apzc->SetFrameMetrics(GetPinchableFrameMetrics());
    MakeApzcZoomable();

    if (aShouldTriggerPinch) {
      EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
      EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);
    } else {
      EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtMost(2));
      EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(0);
    }

    int touchInputId = 0;
    if (mGestureBehavior == AsyncPanZoomController::USE_GESTURE_DETECTOR) {
      ApzcPinchWithTouchInputAndCheckStatus(apzc, 250, 300, 1.25, touchInputId, aShouldTriggerPinch, aAllowedTouchBehaviors);
    } else {
      ApzcPinchWithPinchInputAndCheckStatus(apzc, 250, 300, 1.25, aShouldTriggerPinch);
    }

    FrameMetrics fm = apzc->GetFrameMetrics();

    if (aShouldTriggerPinch) {
      
      EXPECT_EQ(2.5f, fm.GetZoom().scale);
      EXPECT_EQ(305, fm.GetScrollOffset().x);
      EXPECT_EQ(310, fm.GetScrollOffset().y);
    } else {
      
      
      EXPECT_EQ(2.0f, fm.GetZoom().scale);
      EXPECT_EQ(300, fm.GetScrollOffset().x);
      EXPECT_EQ(300, fm.GetScrollOffset().y);
    }

    
    
    fm.SetZoom(CSSToScreenScale(2.0));
    fm.SetScrollOffset(CSSPoint(930, 5));
    apzc->SetFrameMetrics(fm);
    

    if (mGestureBehavior == AsyncPanZoomController::USE_GESTURE_DETECTOR) {
      ApzcPinchWithTouchInputAndCheckStatus(apzc, 250, 300, 0.5, touchInputId, aShouldTriggerPinch, aAllowedTouchBehaviors);
    } else {
      ApzcPinchWithPinchInputAndCheckStatus(apzc, 250, 300, 0.5, aShouldTriggerPinch);
    }

    fm = apzc->GetFrameMetrics();

    if (aShouldTriggerPinch) {
      
      EXPECT_EQ(1.0f, fm.GetZoom().scale);
      EXPECT_EQ(880, fm.GetScrollOffset().x);
      EXPECT_EQ(0, fm.GetScrollOffset().y);
    } else {
      EXPECT_EQ(2.0f, fm.GetZoom().scale);
      EXPECT_EQ(930, fm.GetScrollOffset().x);
      EXPECT_EQ(5, fm.GetScrollOffset().y);
    }
  }
};

class APZCPinchGestureDetectorTester : public APZCPinchTester {
public:
  APZCPinchGestureDetectorTester()
    : APZCPinchTester(AsyncPanZoomController::USE_GESTURE_DETECTOR)
  {
  }
};

TEST_F(APZCPinchTester, Pinch_DefaultGestures_NoTouchAction) {
  DoPinchTest(true);
}

TEST_F(APZCPinchGestureDetectorTester, Pinch_UseGestureDetector_NoTouchAction) {
  DoPinchTest(true);
}

TEST_F(APZCPinchGestureDetectorTester, Pinch_UseGestureDetector_TouchActionNone) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  nsTArray<uint32_t> behaviors;
  behaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::NONE);
  behaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::NONE);
  DoPinchTest(false, &behaviors);
}

TEST_F(APZCPinchGestureDetectorTester, Pinch_UseGestureDetector_TouchActionZoom) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  nsTArray<uint32_t> behaviors;
  behaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
  behaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
  DoPinchTest(true, &behaviors);
}

TEST_F(APZCPinchGestureDetectorTester, Pinch_UseGestureDetector_TouchActionNotAllowZoom) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  nsTArray<uint32_t> behaviors;
  behaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
  behaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
  DoPinchTest(false, &behaviors);
}

TEST_F(APZCPinchGestureDetectorTester, Pinch_PreventDefault) {
  FrameMetrics originalMetrics = GetPinchableFrameMetrics();
  apzc->SetFrameMetrics(originalMetrics);

  SetMayHaveTouchListeners();
  MakeApzcZoomable();

  int touchInputId = 0;
  ApzcPinchWithTouchInput(apzc, 250, 300, 1.25, touchInputId);

  
  apzc->ContentReceivedTouch(true);

  
  
  EXPECT_LE(1, mcc->RunThroughDelayedTasks());

  
  FrameMetrics fm = apzc->GetFrameMetrics();
  EXPECT_EQ(originalMetrics.GetZoom().scale, fm.GetZoom().scale);
  EXPECT_EQ(originalMetrics.GetScrollOffset().x, fm.GetScrollOffset().x);
  EXPECT_EQ(originalMetrics.GetScrollOffset().y, fm.GetScrollOffset().y);

  apzc->AssertStateIsReset();
}

TEST_F(APZCBasicTester, Overzoom) {
  
  FrameMetrics fm;
  fm.mCompositionBounds = ParentLayerRect(0, 0, 100, 100);
  fm.mScrollableRect = CSSRect(0, 0, 125, 150);
  fm.SetScrollOffset(CSSPoint(10, 0));
  fm.SetZoom(CSSToScreenScale(1.0));
  apzc->SetFrameMetrics(fm);

  MakeApzcZoomable();

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  ApzcPinchWithPinchInputAndCheckStatus(apzc, 50, 50, 0.5, true);

  fm = apzc->GetFrameMetrics();
  EXPECT_EQ(0.8f, fm.GetZoom().scale);
  
  
  EXPECT_LT(abs(fm.GetScrollOffset().x), 1e-5);
  EXPECT_LT(abs(fm.GetScrollOffset().y), 1e-5);
}

TEST_F(APZCBasicTester, SimpleTransform) {
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

  EXPECT_EQ(ScreenPoint(), pointOut);
  EXPECT_EQ(ViewTransform(), viewTransformOut);
}


TEST_F(APZCBasicTester, ComplexTransform) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<TestAsyncPanZoomController> childApzc = new TestAsyncPanZoomController(0, mcc, tm);

  const char* layerTreeSyntax = "c(c)";
  
  nsIntRegion layerVisibleRegion[] = {
    nsIntRegion(nsIntRect(0, 0, 300, 300)),
    nsIntRegion(nsIntRect(0, 0, 150, 300)),
  };
  Matrix4x4 transforms[] = {
    Matrix4x4(),
    Matrix4x4(),
  };
  transforms[0].ScalePost(0.5f, 0.5f, 1.0f); 
  transforms[1].ScalePost(2.0f, 1.0f, 1.0f); 

  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, lm, layers);

  FrameMetrics metrics;
  metrics.mCompositionBounds = ParentLayerRect(0, 0, 24, 24);
  metrics.mDisplayPort = CSSRect(-1, -1, 6, 6);
  metrics.SetScrollOffset(CSSPoint(10, 10));
  metrics.mScrollableRect = CSSRect(0, 0, 50, 50);
  metrics.mCumulativeResolution = LayoutDeviceToLayerScale(2);
  metrics.mResolution = ParentLayerToLayerScale(2);
  metrics.SetZoom(CSSToScreenScale(6));
  metrics.mDevPixelsPerCSSPixel = CSSToLayoutDeviceScale(3);
  metrics.SetScrollId(FrameMetrics::START_SCROLL_ID);

  FrameMetrics childMetrics = metrics;
  childMetrics.SetScrollId(FrameMetrics::START_SCROLL_ID + 1);

  layers[0]->SetFrameMetrics(metrics);
  layers[1]->SetFrameMetrics(childMetrics);

  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  

  
  apzc->SetFrameMetrics(metrics);
  apzc->NotifyLayersUpdated(metrics, true);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(ParentLayerToScreenScale(2), ScreenPoint()), viewTransformOut);
  EXPECT_EQ(ScreenPoint(60, 60), pointOut);

  childApzc->SetFrameMetrics(childMetrics);
  childApzc->NotifyLayersUpdated(childMetrics, true);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(ParentLayerToScreenScale(2), ScreenPoint()), viewTransformOut);
  EXPECT_EQ(ScreenPoint(60, 60), pointOut);

  
  metrics.ScrollBy(CSSPoint(5, 0));
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(ParentLayerToScreenScale(2), ScreenPoint(-30, 0)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  childMetrics.ScrollBy(CSSPoint(5, 0));
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(ParentLayerToScreenScale(2), ScreenPoint(-30, 0)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  
  metrics.ZoomBy(1.5f);
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(ParentLayerToScreenScale(3), ScreenPoint(-45, 0)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);

  childMetrics.ZoomBy(1.5f);
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(ParentLayerToScreenScale(3), ScreenPoint(-45, 0)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);
}

class APZCPanningTester : public APZCBasicTester {
protected:
  void DoPanTest(bool aShouldTriggerScroll, bool aShouldBeConsumed, uint32_t aBehavior)
  {
    if (aShouldTriggerScroll) {
      EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
      EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);
    } else {
      EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(0);
      EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(0);
    }

    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;
    ScreenPoint pointOut;
    ViewTransform viewTransformOut;

    nsTArray<uint32_t> allowedTouchBehaviors;
    allowedTouchBehaviors.AppendElement(aBehavior);

    
    ApzcPanAndCheckStatus(apzc, time, touchStart, touchEnd, aShouldBeConsumed, &allowedTouchBehaviors);
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    if (aShouldTriggerScroll) {
      EXPECT_EQ(ScreenPoint(0, -(touchEnd-touchStart)), pointOut);
      EXPECT_NE(ViewTransform(), viewTransformOut);
    } else {
      EXPECT_EQ(ScreenPoint(), pointOut);
      EXPECT_EQ(ViewTransform(), viewTransformOut);
    }

    
    
    apzc->CancelAnimation();

    
    ApzcPanAndCheckStatus(apzc, time, touchEnd, touchStart, aShouldBeConsumed, &allowedTouchBehaviors);
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    EXPECT_EQ(ScreenPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);
  }

  void DoPanWithPreventDefaultTest()
  {
    SetMayHaveTouchListeners();

    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;
    ScreenPoint pointOut;
    ViewTransform viewTransformOut;

    
    nsTArray<uint32_t> allowedTouchBehaviors;
    allowedTouchBehaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
    ApzcPanAndCheckStatus(apzc, time, touchStart, touchEnd, true, &allowedTouchBehaviors);

    
    
    apzc->ContentReceivedTouch(true);
    
    
    EXPECT_LE(1, mcc->RunThroughDelayedTasks());

    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
    EXPECT_EQ(ScreenPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);

    apzc->AssertStateIsReset();
  }
};

TEST_F(APZCPanningTester, Pan) {
  DoPanTest(true, true, mozilla::layers::AllowedTouchBehavior::NONE);
}









TEST_F(APZCPanningTester, PanWithTouchActionAuto) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(true, true, mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                      | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
}

TEST_F(APZCPanningTester, PanWithTouchActionNone) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(false, false, 0);
}

TEST_F(APZCPanningTester, PanWithTouchActionPanX) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(false, true, mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN);
}

TEST_F(APZCPanningTester, PanWithTouchActionPanY) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(true, true, mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
}

TEST_F(APZCPanningTester, PanWithPreventDefaultAndTouchAction) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanWithPreventDefaultTest();
}

TEST_F(APZCPanningTester, PanWithPreventDefault) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);
  DoPanWithPreventDefaultTest();
}

TEST_F(APZCBasicTester, Fling) {
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  int time = 0;
  int touchStart = 50;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  ApzcPan(apzc, time, touchStart, touchEnd);
  ScreenPoint lastPoint;
  for (int i = 1; i < 50; i+=1) {
    apzc->SampleContentTransformForFrame(testStartTime+TimeDuration::FromMilliseconds(i), &viewTransformOut, pointOut);
    EXPECT_GT(pointOut.y, lastPoint.y);
    lastPoint = pointOut;
  }
}

TEST_F(APZCBasicTester, FlingIntoOverscroll) {
  
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  int time = 0;
  ApzcPanNoFling(apzc, time, 50, 25);

  
  
  
  ApzcPan(apzc, time, 25, 45);
  const TimeDuration increment = TimeDuration::FromMilliseconds(1);
  bool reachedOverscroll = false;
  bool recoveredFromOverscroll = false;
  while (apzc->AdvanceAnimations(testStartTime)) {
    if (!reachedOverscroll && apzc->IsOverscrolled()) {
      reachedOverscroll = true;
    }
    if (reachedOverscroll && !apzc->IsOverscrolled()) {
      recoveredFromOverscroll = true;
    }
    testStartTime += increment;
  }
  EXPECT_TRUE(reachedOverscroll);
  EXPECT_TRUE(recoveredFromOverscroll);
}

TEST_F(APZCBasicTester, OverScrollPanning) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  ApzcPan(apzc, time, touchStart, touchEnd);
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  
  
  
  
  

  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  
  apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(10000), &viewTransformOut, pointOut);
  EXPECT_EQ(ScreenPoint(0, 90), pointOut);
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  
  apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(20000), &viewTransformOut, pointOut);
  EXPECT_EQ(ScreenPoint(0, 90), pointOut);
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(30000), &viewTransformOut, pointOut);
  EXPECT_EQ(ScreenPoint(0, 90), pointOut);
  EXPECT_FALSE(apzc->IsOverscrolled());

  apzc->AssertStateIsReset();
}

TEST_F(APZCBasicTester, OverScrollAbort) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  ApzcPan(apzc, time, touchStart, touchEnd);
  EXPECT_TRUE(apzc->IsOverscrolled());

  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  
  
  apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(10000), &viewTransformOut, pointOut);
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  
  apzc->CancelAnimation();
  EXPECT_FALSE(apzc->IsOverscrolled());
  apzc->AssertStateIsReset();
}

TEST_F(APZCBasicTester, OverScrollPanningAbort) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  ApzcPan(apzc, time, touchStart, touchEnd,
          true);                   
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  
  
  apzc->CancelAnimation();
  EXPECT_FALSE(apzc->IsOverscrolled());
  apzc->AssertStateIsReset();
}


class APZCFlingStopTester : public APZCGestureDetectorTester {
protected:
  
  
  
  
  
  
  void DoFlingStopTest(bool aSlow) {
    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;

    
    ApzcPan(apzc, time, touchStart, touchEnd);
    
    while (mcc->RunThroughDelayedTasks());

    
    
    
    int timeDelta = aSlow ? 2000 : 10;
    int tapCallsExpected = aSlow ? 1 : 0;

    
    ScreenPoint pointOut;
    ViewTransform viewTransformOut;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(timeDelta), &viewTransformOut, pointOut);

    
    
    EXPECT_CALL(*mcc, HandleSingleTap(_, 0, apzc->GetGuid())).Times(tapCallsExpected);
    ApzcTap(apzc, 10, 10, time, 0);
    while (mcc->RunThroughDelayedTasks());

    
    ScreenPoint finalPointOut;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(timeDelta + 1000), &viewTransformOut, finalPointOut);
    EXPECT_EQ(pointOut.x, finalPointOut.x);
    EXPECT_EQ(pointOut.y, finalPointOut.y);

    apzc->AssertStateIsReset();
  }

  void DoFlingStopWithSlowListener(bool aPreventDefault) {
    SetMayHaveTouchListeners();

    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;

    
    ApzcPan(apzc, time, touchStart, touchEnd);
    apzc->ContentReceivedTouch(false);
    while (mcc->RunThroughDelayedTasks());

    
    ScreenPoint point, finalPoint;
    ViewTransform viewTransform;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(10), &viewTransform, point);
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(20), &viewTransform, finalPoint);
    EXPECT_GT(finalPoint.y, point.y);

    
    ApzcDown(apzc, 10, 10, time);

    
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(30), &viewTransform, point);
    EXPECT_EQ(finalPoint.x, point.x);
    EXPECT_EQ(finalPoint.y, point.y);

    
    
    apzc->ContentReceivedTouch(aPreventDefault);
    while (mcc->RunThroughDelayedTasks());

    
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(100), &viewTransform, point);
    EXPECT_EQ(finalPoint.x, point.x);
    EXPECT_EQ(finalPoint.y, point.y);

    
    ApzcUp(apzc, 10, 10, time);
    while (mcc->RunThroughDelayedTasks());

    apzc->AssertStateIsReset();
  }
};

TEST_F(APZCFlingStopTester, FlingStop) {
  DoFlingStopTest(false);
}

TEST_F(APZCFlingStopTester, FlingStopTap) {
  DoFlingStopTest(true);
}

TEST_F(APZCFlingStopTester, FlingStopSlowListener) {
  DoFlingStopWithSlowListener(false);
}

TEST_F(APZCFlingStopTester, FlingStopPreventDefault) {
  DoFlingStopWithSlowListener(true);
}

TEST_F(APZCGestureDetectorTester, ShortPress) {
  MakeApzcUnzoomable();

  int time = 0;
  ApzcTapAndCheckStatus(apzc, 10, 10, time, 100);
  
  
  mcc->ClearDelayedTask();
  mcc->ClearDelayedTask();

  
  
  mcc->CheckHasDelayedTask();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
  mcc->RunDelayedTask();

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, MediumPress) {
  MakeApzcUnzoomable();

  int time = 0;
  ApzcTapAndCheckStatus(apzc, 10, 10, time, 400);
  
  
  mcc->ClearDelayedTask();
  mcc->ClearDelayedTask();

  
  
  mcc->CheckHasDelayedTask();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
  mcc->RunDelayedTask();

  apzc->AssertStateIsReset();
}

class APZCLongPressTester : public APZCGestureDetectorTester {
protected:
  void DoLongPressTest(uint32_t aBehavior) {
    MakeApzcUnzoomable();

    int time = 0;

    nsEventStatus status = ApzcDown(apzc, 10, 10, time);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    if (gfxPrefs::TouchActionEnabled()) {
      
      nsTArray<uint32_t> allowedTouchBehaviors;
      allowedTouchBehaviors.AppendElement(aBehavior);
      apzc->SetAllowedTouchBehavior(allowedTouchBehaviors);
    }
    
    apzc->ContentReceivedTouch(false);

    MockFunction<void(std::string checkPointName)> check;

    {
      InSequence s;

      EXPECT_CALL(check, Call("preHandleLongTap"));
      EXPECT_CALL(*mcc, HandleLongTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
      EXPECT_CALL(check, Call("postHandleLongTap"));

      EXPECT_CALL(check, Call("preHandleLongTapUp"));
      EXPECT_CALL(*mcc, HandleLongTapUp(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
      EXPECT_CALL(check, Call("postHandleLongTapUp"));
    }

    
    mcc->CheckHasDelayedTask();

    
    check.Call("preHandleLongTap");
    mcc->RunDelayedTask();
    check.Call("postHandleLongTap");

    
    mcc->DestroyOldestTask();

    
    
    
    
    
    apzc->ContentReceivedTouch(false);
    mcc->CheckHasDelayedTask();
    mcc->RunDelayedTask();

    time += 1000;

    
    
    check.Call("preHandleLongTapUp");
    status = ApzcUp(apzc, 10, 10, time);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);
    check.Call("postHandleLongTapUp");

    apzc->AssertStateIsReset();
  }

  void DoLongPressPreventDefaultTest(uint32_t aBehavior) {
    MakeApzcUnzoomable();

    EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(0);
    EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(0);

    int touchX = 10,
        touchStartY = 10,
        touchEndY = 50;

    int time = 0;
    nsEventStatus status = ApzcDown(apzc, touchX, touchStartY, time);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    if (gfxPrefs::TouchActionEnabled()) {
      
      nsTArray<uint32_t> allowedTouchBehaviors;
      allowedTouchBehaviors.AppendElement(aBehavior);
      apzc->SetAllowedTouchBehavior(allowedTouchBehaviors);
    }
    
    apzc->ContentReceivedTouch(false);

    MockFunction<void(std::string checkPointName)> check;

    {
      InSequence s;

      EXPECT_CALL(check, Call("preHandleLongTap"));
      EXPECT_CALL(*mcc, HandleLongTap(CSSPoint(touchX, touchStartY), 0, apzc->GetGuid())).Times(1);
      EXPECT_CALL(check, Call("postHandleLongTap"));
    }

    mcc->CheckHasDelayedTask();

    
    check.Call("preHandleLongTap");
    mcc->RunDelayedTask();
    check.Call("postHandleLongTap");

    
    mcc->DestroyOldestTask();

    
    
    
    
    
    apzc->ContentReceivedTouch(true);
    mcc->CheckHasDelayedTask();
    mcc->RunDelayedTask();

    time += 1000;

    MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, time, TimeStamp(), 0);
    mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(touchX, touchEndY), ScreenSize(0, 0), 0, 0));
    status = apzc->ReceiveInputEvent(mti);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    EXPECT_CALL(*mcc, HandleLongTapUp(CSSPoint(touchX, touchEndY), 0, apzc->GetGuid())).Times(0);
    status = ApzcUp(apzc, touchX, touchEndY, time);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    ScreenPoint pointOut;
    ViewTransform viewTransformOut;
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    EXPECT_EQ(ScreenPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);

    apzc->AssertStateIsReset();
  }
};

TEST_F(APZCLongPressTester, LongPress) {
  DoLongPressTest(mozilla::layers::AllowedTouchBehavior::NONE);
}

TEST_F(APZCLongPressTester, LongPressWithTouchAction) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoLongPressTest(mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                  | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN
                  | mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
}

TEST_F(APZCLongPressTester, LongPressPreventDefault) {
  DoLongPressPreventDefaultTest(mozilla::layers::AllowedTouchBehavior::NONE);
}

TEST_F(APZCLongPressTester, LongPressPreventDefaultWithTouchAction) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoLongPressPreventDefaultTest(mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                                | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN
                                | mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
}

static void
ApzcDoubleTap(AsyncPanZoomController* aApzc, int aX, int aY, int& aTime,
              nsEventStatus (*aOutEventStatuses)[4] = nullptr)
{
  nsEventStatus status = ApzcDown(aApzc, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }
  aTime += 10;
  status = ApzcUp(aApzc, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }
  aTime += 10;
  status = ApzcDown(aApzc, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = status;
  }
  aTime += 10;
  status = ApzcUp(aApzc, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }
}

static void
ApzcDoubleTapAndCheckStatus(AsyncPanZoomController* aApzc, int aX, int aY, int& aTime)
{
  nsEventStatus statuses[4];
  ApzcDoubleTap(aApzc, aX, aY, aTime, &statuses);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[0]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[1]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[2]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[3]);
}

TEST_F(APZCGestureDetectorTester, DoubleTap) {
  SetMayHaveTouchListeners();
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);

  int time = 0;
  ApzcDoubleTapAndCheckStatus(apzc, 10, 10, time);

  
  apzc->ContentReceivedTouch(false);
  apzc->ContentReceivedTouch(false);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, DoubleTapNotZoomable) {
  SetMayHaveTouchListeners();
  MakeApzcUnzoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(2);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);

  int time = 0;
  ApzcDoubleTapAndCheckStatus(apzc, 10, 10, time);

  
  apzc->ContentReceivedTouch(false);
  apzc->ContentReceivedTouch(false);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, DoubleTapPreventDefaultFirstOnly) {
  SetMayHaveTouchListeners();
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);

  int time = 0;
  ApzcDoubleTapAndCheckStatus(apzc, 10, 10, time);

  
  apzc->ContentReceivedTouch(true);
  apzc->ContentReceivedTouch(false);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, DoubleTapPreventDefaultBoth) {
  SetMayHaveTouchListeners();
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);

  int time = 0;
  ApzcDoubleTapAndCheckStatus(apzc, 10, 10, time);

  
  apzc->ContentReceivedTouch(true);
  apzc->ContentReceivedTouch(true);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

class APZCTreeManagerTester : public ::testing::Test {
protected:
  virtual void SetUp() {
    gfxPrefs::GetSingleton();
    AsyncPanZoomController::SetThreadAssertionsEnabled(false);

    testStartTime = TimeStamp::Now();
    AsyncPanZoomController::SetFrameTime(testStartTime);

    mcc = new NiceMock<MockContentController>();
    manager = new TestAPZCTreeManager();
  }

  virtual void TearDown() {
    manager->ClearTree();
  }

  TimeStamp testStartTime;
  nsRefPtr<MockContentController> mcc;

  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root;

  nsRefPtr<TestAPZCTreeManager> manager;

protected:
  static void SetScrollableFrameMetrics(Layer* aLayer, FrameMetrics::ViewID aScrollId,
                                        
                                        
                                        CSSRect aScrollableRect = CSSRect(-1, -1, -1, -1)) {
    FrameMetrics metrics;
    metrics.SetScrollId(aScrollId);
    nsIntRect layerBound = aLayer->GetVisibleRegion().GetBounds();
    metrics.mCompositionBounds = ParentLayerRect(layerBound.x, layerBound.y,
                                                 layerBound.width, layerBound.height);
    metrics.mScrollableRect = aScrollableRect;
    metrics.SetScrollOffset(CSSPoint(0, 0));
    aLayer->SetFrameMetrics(metrics);
  }

  void CreateSimpleMultiLayerTree() {
    const char* layerTreeSyntax = "c(tt)";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,100,100)),
      nsIntRegion(nsIntRect(0,0,100,50)),
      nsIntRegion(nsIntRect(0,50,100,50)),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
  }
};

class APZHitTestingTester : public APZCTreeManagerTester {
protected:
  Matrix4x4 transformToApzc;
  Matrix4x4 transformToGecko;

  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint) {
    nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(aPoint, nullptr);
    if (hit) {
      manager->GetInputTransforms(hit.get(), transformToApzc, transformToGecko);
    }
    return hit.forget();
  }

protected:
  void CreateHitTesting1LayerTree() {
    const char* layerTreeSyntax = "c(ttcc)";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,100,100)),
      nsIntRegion(nsIntRect(0,0,100,100)),
      nsIntRegion(nsIntRect(10,10,20,20)),
      nsIntRegion(nsIntRect(10,10,20,20)),
      nsIntRegion(nsIntRect(5,5,20,20)),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
  }

  void CreateHitTesting2LayerTree() {
    const char* layerTreeSyntax = "c(cc(c))";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,100,100)),
      nsIntRegion(nsIntRect(10,10,40,40)),
      nsIntRegion(nsIntRect(10,60,40,40)),
      nsIntRegion(nsIntRect(10,60,40,40)),
    };
    Matrix4x4 transforms[] = {
      Matrix4x4(),
      Matrix4x4(),
      Matrix4x4().Scale(2, 1, 1),
      Matrix4x4(),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, lm, layers);

    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 80, 80));
    SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 0, 80, 80));
  }

  void CreateComplexMultiLayerTree() {
    const char* layerTreeSyntax = "c(tc(t)tc(c(t)t))";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,300,400)),      
      nsIntRegion(nsIntRect(0,0,100,100)),      
      nsIntRegion(nsIntRect(50,50,200,300)),    
      nsIntRegion(nsIntRect(50,50,200,300)),    
      nsIntRegion(nsIntRect(0,200,100,100)),    
      nsIntRegion(nsIntRect(200,0,100,400)),    
      nsIntRegion(nsIntRect(200,0,100,200)),    
      nsIntRegion(nsIntRect(200,0,100,200)),    
      nsIntRegion(nsIntRect(200,200,100,200)),  
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID);
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID);
    SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[6], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[7], FrameMetrics::START_SCROLL_ID + 2);
    SetScrollableFrameMetrics(layers[8], FrameMetrics::START_SCROLL_ID + 3);
  }
};


TEST_F(APZHitTestingTester, HitTesting1) {
  CreateHitTesting1LayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  
  nsRefPtr<AsyncPanZoomController> hit = GetTargetAPZC(ScreenPoint(20, 20));
  AsyncPanZoomController* nullAPZC = nullptr;
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(Matrix4x4(), transformToApzc);
  EXPECT_EQ(Matrix4x4(), transformToGecko);

  uint32_t paintSequenceNumber = 0;

  
  SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, paintSequenceNumber++);
  hit = GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(root->GetAsyncPanZoomController(0), hit.get());
  
  EXPECT_EQ(Point(15, 15), transformToApzc * Point(15, 15));
  EXPECT_EQ(Point(15, 15), transformToGecko * Point(15, 15));

  
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, paintSequenceNumber++);
  EXPECT_NE(root->GetAsyncPanZoomController(0), layers[3]->GetAsyncPanZoomController(0));
  hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(layers[3]->GetAsyncPanZoomController(0), hit.get());
  
  EXPECT_EQ(Point(25, 25), transformToApzc * Point(25, 25));
  EXPECT_EQ(Point(25, 25), transformToGecko * Point(25, 25));

  
  
  hit = GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(root->GetAsyncPanZoomController(0), hit.get());

  
  SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 2);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, paintSequenceNumber++);
  hit = GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(layers[4]->GetAsyncPanZoomController(0), hit.get());
  
  EXPECT_EQ(Point(15, 15), transformToApzc * Point(15, 15));
  EXPECT_EQ(Point(15, 15), transformToGecko * Point(15, 15));

  
  hit = GetTargetAPZC(ScreenPoint(90, 90));
  EXPECT_EQ(root->GetAsyncPanZoomController(0), hit.get());
  
  EXPECT_EQ(Point(90, 90), transformToApzc * Point(90, 90));
  EXPECT_EQ(Point(90, 90), transformToGecko * Point(90, 90));

  
  hit = GetTargetAPZC(ScreenPoint(1000, 10));
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(Matrix4x4(), transformToApzc);
  EXPECT_EQ(Matrix4x4(), transformToGecko);
  hit = GetTargetAPZC(ScreenPoint(-1000, 10));
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(Matrix4x4(), transformToApzc);
  EXPECT_EQ(Matrix4x4(), transformToGecko);
}


TEST_F(APZHitTestingTester, HitTesting2) {
  CreateHitTesting2LayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);

  
  
  
  
  

  AsyncPanZoomController* apzcroot = root->GetAsyncPanZoomController(0);
  AsyncPanZoomController* apzc1 = layers[1]->GetAsyncPanZoomController(0);
  AsyncPanZoomController* apzc3 = layers[3]->GetAsyncPanZoomController(0);

  
  nsRefPtr<AsyncPanZoomController> hit = GetTargetAPZC(ScreenPoint(75, 25));
  EXPECT_EQ(apzcroot, hit.get());
  EXPECT_EQ(Point(75, 25), transformToApzc * Point(75, 25));
  EXPECT_EQ(Point(75, 25), transformToGecko * Point(75, 25));

  
  
  
  
  
  
  
  hit = GetTargetAPZC(ScreenPoint(15, 75));
  EXPECT_EQ(apzcroot, hit.get());
  EXPECT_EQ(Point(15, 75), transformToApzc * Point(15, 75));
  EXPECT_EQ(Point(15, 75), transformToGecko * Point(15, 75));

  
  hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(apzc1, hit.get());
  EXPECT_EQ(Point(25, 25), transformToApzc * Point(25, 25));
  EXPECT_EQ(Point(25, 25), transformToGecko * Point(25, 25));

  
  hit = GetTargetAPZC(ScreenPoint(25, 75));
  EXPECT_EQ(apzc3, hit.get());
  
  EXPECT_EQ(Point(12.5, 75), transformToApzc * Point(25, 75));
  
  EXPECT_EQ(Point(25, 75), transformToGecko * Point(12.5, 75));

  
  
  hit = GetTargetAPZC(ScreenPoint(75, 75));
  EXPECT_EQ(apzc3, hit.get());
  
  EXPECT_EQ(Point(37.5, 75), transformToApzc * Point(75, 75));
  
  EXPECT_EQ(Point(75, 75), transformToGecko * Point(37.5, 75));

  
  
  
  int time = 0;
  
  EXPECT_CALL(*mcc, PostDelayedTask(_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  
  
  
  ApzcPanNoFling(apzcroot, time, 100, 50);

  
  hit = GetTargetAPZC(ScreenPoint(75, 75));
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(Point(75, 75), transformToApzc * Point(75, 75));
  
  
  
  EXPECT_EQ(Point(75, 75), transformToGecko * Point(75, 75));

  
  hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(apzc3, hit.get());
  
  
  EXPECT_EQ(Point(12.5, 75), transformToApzc * Point(25, 25));
  
  
  EXPECT_EQ(Point(25, 25), transformToGecko * Point(12.5, 75));

  
  
  
  
  ApzcPanNoFling(apzcroot, time, 100, 50);

  
  hit = GetTargetAPZC(ScreenPoint(75, 75));
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(Point(75, 75), transformToApzc * Point(75, 75));
  
  
  EXPECT_EQ(Point(75, 125), transformToGecko * Point(75, 75));

  
  hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(Point(25, 25), transformToApzc * Point(25, 25));
  
  
  EXPECT_EQ(Point(25, 75), transformToGecko * Point(25, 25));
}

TEST_F(APZCTreeManagerTester, ScrollableThebesLayers) {
  CreateSimpleMultiLayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  
  SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID);
  SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);

  AsyncPanZoomController* nullAPZC = nullptr;
  
  EXPECT_FALSE(layers[0]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, layers[1]->GetAsyncPanZoomController(0));
  EXPECT_NE(nullAPZC, layers[2]->GetAsyncPanZoomController(0));
  EXPECT_EQ(layers[1]->GetAsyncPanZoomController(0), layers[2]->GetAsyncPanZoomController(0));

  
  SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);
  EXPECT_NE(layers[1]->GetAsyncPanZoomController(0), layers[2]->GetAsyncPanZoomController(0));

  
  
  SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);
  EXPECT_EQ(layers[1]->GetAsyncPanZoomController(0), layers[2]->GetAsyncPanZoomController(0));
}

TEST_F(APZHitTestingTester, ComplexMultiLayerTree) {
  CreateComplexMultiLayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);

  AsyncPanZoomController* nullAPZC = nullptr;
  
  EXPECT_FALSE(layers[0]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, layers[1]->GetAsyncPanZoomController(0));
  EXPECT_NE(nullAPZC, layers[2]->GetAsyncPanZoomController(0));
  EXPECT_FALSE(layers[3]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, layers[4]->GetAsyncPanZoomController(0));
  EXPECT_FALSE(layers[5]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, layers[6]->GetAsyncPanZoomController(0));
  EXPECT_NE(nullAPZC, layers[7]->GetAsyncPanZoomController(0));
  EXPECT_NE(nullAPZC, layers[8]->GetAsyncPanZoomController(0));
  
  EXPECT_EQ(layers[1]->GetAsyncPanZoomController(0), layers[2]->GetAsyncPanZoomController(0));
  EXPECT_EQ(layers[4]->GetAsyncPanZoomController(0), layers[6]->GetAsyncPanZoomController(0));
  
  EXPECT_NE(layers[1]->GetAsyncPanZoomController(0), layers[4]->GetAsyncPanZoomController(0));
  EXPECT_NE(layers[1]->GetAsyncPanZoomController(0), layers[7]->GetAsyncPanZoomController(0));
  EXPECT_NE(layers[1]->GetAsyncPanZoomController(0), layers[8]->GetAsyncPanZoomController(0));
  EXPECT_NE(layers[4]->GetAsyncPanZoomController(0), layers[7]->GetAsyncPanZoomController(0));
  EXPECT_NE(layers[4]->GetAsyncPanZoomController(0), layers[8]->GetAsyncPanZoomController(0));
  EXPECT_NE(layers[7]->GetAsyncPanZoomController(0), layers[8]->GetAsyncPanZoomController(0));

  nsRefPtr<AsyncPanZoomController> hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(layers[1]->GetAsyncPanZoomController(0), hit.get());
  hit = GetTargetAPZC(ScreenPoint(275, 375));
  EXPECT_EQ(layers[8]->GetAsyncPanZoomController(0), hit.get());
  hit = GetTargetAPZC(ScreenPoint(250, 100));
  EXPECT_EQ(layers[7]->GetAsyncPanZoomController(0), hit.get());
}

class APZOverscrollHandoffTester : public APZCTreeManagerTester {
protected:
  UniquePtr<ScopedLayerTreeRegistration> registration;
  TestAsyncPanZoomController* rootApzc;

  void SetScrollHandoff(Layer* aChild, Layer* aParent) {
    FrameMetrics metrics = aChild->GetFrameMetrics(0);
    metrics.SetScrollParentId(aParent->GetFrameMetrics(0).GetScrollId());
    aChild->SetFrameMetrics(metrics);
  }

  void CreateOverscrollHandoffLayerTree1() {
    const char* layerTreeSyntax = "c(c)";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 50, 100, 50))
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 100, 100));
    SetScrollHandoff(layers[1], root);
    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);
    rootApzc = (TestAsyncPanZoomController*)root->GetAsyncPanZoomController(0);
  }

  void CreateOverscrollHandoffLayerTree2() {
    const char* layerTreeSyntax = "c(c(c))";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 50, 100, 50))
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 2, CSSRect(-100, -100, 200, 200));
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 100, 100));
    SetScrollHandoff(layers[1], root);
    SetScrollHandoff(layers[2], layers[1]);
    
    
    MOZ_ASSERT(registration);
    manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);
    rootApzc = (TestAsyncPanZoomController*)root->GetAsyncPanZoomController(0);
  }

  void CreateOverscrollHandoffLayerTree3() {
    const char* layerTreeSyntax = "c(c(c)c(c))";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),  
      nsIntRegion(nsIntRect(0, 0, 100, 50)),   
      nsIntRegion(nsIntRect(0, 0, 100, 50)),   
      nsIntRegion(nsIntRect(0, 50, 100, 50)),  
      nsIntRegion(nsIntRect(0, 50, 100, 50))   
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 100, 100));
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 100, 100));
    SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 50, 100, 100));
    SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 3, CSSRect(0, 50, 100, 100));
    SetScrollHandoff(layers[2], layers[1]);
    SetScrollHandoff(layers[4], layers[3]);
    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);
  }

  void CreateScrollgrabLayerTree() {
    const char* layerTreeSyntax = "c(c)";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),  
      nsIntRegion(nsIntRect(0, 20, 100, 80))   
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 100, 120));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 100, 200));
    SetScrollHandoff(layers[1], root);
    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);
    rootApzc = (TestAsyncPanZoomController*)root->GetAsyncPanZoomController(0);
    rootApzc->GetFrameMetrics().SetHasScrollgrab(true);
  }
};




TEST_F(APZOverscrollHandoffTester, DeferredInputEventProcessing) {
  
  CreateOverscrollHandoffLayerTree1();

  TestAsyncPanZoomController* childApzc = (TestAsyncPanZoomController*)layers[1]->GetAsyncPanZoomController(0);

  
  
  childApzc->GetFrameMetrics().mMayHaveTouchListeners = true;

  
  int time = 0;
  ApzcPanNoFling(childApzc, time, 90, 30);

  
  childApzc->ContentReceivedTouch(false);

  
  EXPECT_EQ(50, childApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(10, rootApzc->GetFrameMetrics().GetScrollOffset().y);
}






TEST_F(APZOverscrollHandoffTester, LayerStructureChangesWhileEventsArePending) {
  
  CreateOverscrollHandoffLayerTree1();

  TestAsyncPanZoomController* childApzc = (TestAsyncPanZoomController*)layers[1]->GetAsyncPanZoomController(0);

  
  
  childApzc->GetFrameMetrics().mMayHaveTouchListeners = true;

  
  int time = 0;
  ApzcPanNoFling(childApzc, time, 90, 30);

  
  
  CreateOverscrollHandoffLayerTree2();
  nsRefPtr<Layer> middle = layers[1];
  childApzc->GetFrameMetrics().mMayHaveTouchListeners = true;
  TestAsyncPanZoomController* middleApzc = (TestAsyncPanZoomController*)middle->GetAsyncPanZoomController(0);

  
  ApzcPanNoFling(childApzc, time, 30, 90);

  
  childApzc->ContentReceivedTouch(false);

  
  
  EXPECT_EQ(50, childApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(10, rootApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(0, middleApzc->GetFrameMetrics().GetScrollOffset().y);

  
  childApzc->ContentReceivedTouch(false);

  
  
  EXPECT_EQ(0, childApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(10, rootApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(-10, middleApzc->GetFrameMetrics().GetScrollOffset().y);
}



TEST_F(APZOverscrollHandoffTester, SimultaneousFlings) {
  
  CreateOverscrollHandoffLayerTree3();

  TestAsyncPanZoomController* parent1 = (TestAsyncPanZoomController*)layers[1]->GetAsyncPanZoomController(0);
  TestAsyncPanZoomController* child1 = (TestAsyncPanZoomController*)layers[2]->GetAsyncPanZoomController(0);
  TestAsyncPanZoomController* parent2 = (TestAsyncPanZoomController*)layers[3]->GetAsyncPanZoomController(0);
  TestAsyncPanZoomController* child2 = (TestAsyncPanZoomController*)layers[4]->GetAsyncPanZoomController(0);

  
  int time = 0;
  ApzcPan(child2, time, 45, 5);

  
  ApzcPan(child1, time, 95, 55);

  
  child1->AssertStateIsFling();
  child2->AssertStateIsFling();

  
  TimeStamp timestamp = TimeStamp::Now();
  child1->AdvanceAnimationsUntilEnd(timestamp);
  child2->AdvanceAnimationsUntilEnd(timestamp);

  
  child1->AssertStateIsReset();
  parent1->AssertStateIsFling();
  child2->AssertStateIsReset();
  parent2->AssertStateIsFling();
}

TEST_F(APZOverscrollHandoffTester, Scrollgrab) {
  
  CreateScrollgrabLayerTree();

  TestAsyncPanZoomController* childApzc = (TestAsyncPanZoomController*)layers[1]->GetAsyncPanZoomController(0);

  
  
  int time = 0;
  ApzcPan(childApzc, time, 80, 45);

  
  EXPECT_EQ(20, rootApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(15, childApzc->GetFrameMetrics().GetScrollOffset().y);
}

TEST_F(APZOverscrollHandoffTester, ScrollgrabFling) {
  
  CreateScrollgrabLayerTree();

  TestAsyncPanZoomController* childApzc = (TestAsyncPanZoomController*)layers[1]->GetAsyncPanZoomController(0);

  
  int time = 0;
  ApzcPan(childApzc, time, 80, 70);

  
  rootApzc->AssertStateIsFling();
  childApzc->AssertStateIsReset();
}


class TaskRunMetrics {
public:
  TaskRunMetrics()
    : mRunCount(0)
    , mCancelCount(0)
  {}

  void IncrementRunCount() {
    mRunCount++;
  }

  void IncrementCancelCount() {
    mCancelCount++;
  }

  int GetAndClearRunCount() {
    int runCount = mRunCount;
    mRunCount = 0;
    return runCount;
  }

  int GetAndClearCancelCount() {
    int cancelCount = mCancelCount;
    mCancelCount = 0;
    return cancelCount;
  }

private:
  int mRunCount;
  int mCancelCount;
};

class MockTask : public CancelableTask {
public:
  explicit MockTask(TaskRunMetrics& aMetrics)
    : mMetrics(aMetrics)
  {}

  virtual void Run() {
    mMetrics.IncrementRunCount();
  }

  virtual void Cancel() {
    mMetrics.IncrementCancelCount();
  }

private:
  TaskRunMetrics& mMetrics;
};

class APZTaskThrottlerTester : public ::testing::Test {
public:
  APZTaskThrottlerTester()
  {
    now = TimeStamp::Now();
    throttler = MakeUnique<TaskThrottler>(now, TimeDuration::FromMilliseconds(100));
  }

protected:
  TimeStamp Advance(int aMillis = 5)
  {
    now = now + TimeDuration::FromMilliseconds(aMillis);
    return now;
  }

  UniquePtr<CancelableTask> NewTask()
  {
    return MakeUnique<MockTask>(metrics);
  }

  TimeStamp now;
  UniquePtr<TaskThrottler> throttler;
  TaskRunMetrics metrics;
};

TEST_F(APZTaskThrottlerTester, BasicTest) {
  
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  EXPECT_EQ(1, metrics.GetAndClearRunCount());

  
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  EXPECT_EQ(0, metrics.GetAndClearRunCount());
  throttler->TaskComplete(Advance());                           
  EXPECT_EQ(1, metrics.GetAndClearRunCount());
  EXPECT_EQ(0, metrics.GetAndClearCancelCount());

  
  
  
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  EXPECT_EQ(0, metrics.GetAndClearRunCount());
  EXPECT_EQ(4, metrics.GetAndClearCancelCount());

  throttler->TaskComplete(Advance());                           
  EXPECT_EQ(1, metrics.GetAndClearRunCount());
  throttler->TaskComplete(Advance());                           
  EXPECT_EQ(0, metrics.GetAndClearRunCount());
  EXPECT_EQ(0, metrics.GetAndClearCancelCount());
}

TEST_F(APZTaskThrottlerTester, TimeoutTest) {
  
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  EXPECT_EQ(1, metrics.GetAndClearRunCount());

  
  
  
  throttler->PostTask(FROM_HERE, NewTask(), Advance(100));      
  EXPECT_EQ(1, metrics.GetAndClearRunCount());
  throttler->TaskComplete(Advance());                           
  throttler->TaskComplete(Advance());                           
  EXPECT_EQ(0, metrics.GetAndClearRunCount());
  EXPECT_EQ(0, metrics.GetAndClearCancelCount());

  
  
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  EXPECT_EQ(1, metrics.GetAndClearRunCount());
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  throttler->PostTask(FROM_HERE, NewTask(), Advance());         
  EXPECT_EQ(0, metrics.GetAndClearRunCount());
  throttler->PostTask(FROM_HERE, NewTask(), Advance(100));      
  EXPECT_EQ(1, metrics.GetAndClearRunCount());
  EXPECT_EQ(3, metrics.GetAndClearCancelCount());               
  throttler->TaskComplete(Advance());                           
  EXPECT_EQ(0, metrics.GetAndClearRunCount());
  EXPECT_EQ(0, metrics.GetAndClearCancelCount());
}
