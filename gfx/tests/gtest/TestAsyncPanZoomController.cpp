




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/Attributes.h"
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "mozilla/layers/LayerMetricsWrapper.h"
#include "mozilla/layers/APZThreadUtils.h"
#include "mozilla/UniquePtr.h"
#include "apz/src/AsyncPanZoomController.h"
#include "apz/src/HitTestingTreeNode.h"
#include "base/task.h"
#include "Layers.h"
#include "TestLayers.h"
#include "UnitTransforms.h"
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
  MOCK_METHOD2(RequestFlingSnap, void(const FrameMetrics::ViewID& aScrollId, const mozilla::CSSPoint& aDestination));
  MOCK_METHOD2(AcknowledgeScrollUpdate, void(const FrameMetrics::ViewID&, const uint32_t& aScrollGeneration));
  MOCK_METHOD3(HandleDoubleTap, void(const CSSPoint&, Modifiers, const ScrollableLayerGuid&));
  MOCK_METHOD3(HandleSingleTap, void(const CSSPoint&, Modifiers, const ScrollableLayerGuid&));
  MOCK_METHOD4(HandleLongTap, void(const CSSPoint&, Modifiers, const ScrollableLayerGuid&, uint64_t));
  MOCK_METHOD3(SendAsyncScrollDOMEvent, void(bool aIsRoot, const CSSRect &aContentRect, const CSSSize &aScrollableSize));
  MOCK_METHOD2(PostDelayedTask, void(Task* aTask, int aDelayMs));
  MOCK_METHOD3(NotifyAPZStateChange, void(const ScrollableLayerGuid& aGuid, APZStateChange aChange, int aArg));
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

class TestAPZCTreeManager : public APZCTreeManager {
public:
  nsRefPtr<InputQueue> GetInputQueue() const {
    return mInputQueue;
  }

protected:
  AsyncPanZoomController* MakeAPZCInstance(uint64_t aLayersId, GeckoContentController* aController) override;
};

class TestAsyncPanZoomController : public AsyncPanZoomController {
public:
  TestAsyncPanZoomController(uint64_t aLayersId, GeckoContentController* aMcc,
                             TestAPZCTreeManager* aTreeManager,
                             GestureBehavior aBehavior = DEFAULT_GESTURES)
    : AsyncPanZoomController(aLayersId, aTreeManager, aTreeManager->GetInputQueue(), aMcc, aBehavior)
    , mWaitForMainThread(false)
  {}

  nsEventStatus ReceiveInputEvent(const InputData& aEvent, ScrollableLayerGuid* aDummy, uint64_t* aOutInputBlockId) {
    
    
    
    
    return ReceiveInputEvent(aEvent, aOutInputBlockId);
  }

  nsEventStatus ReceiveInputEvent(const InputData& aEvent, uint64_t* aOutInputBlockId) {
    return GetInputQueue()->ReceiveInputEvent(this, !mWaitForMainThread, aEvent, aOutInputBlockId);
  }

  void ContentReceivedInputBlock(uint64_t aInputBlockId, bool aPreventDefault) {
    GetInputQueue()->ContentReceivedInputBlock(aInputBlockId, aPreventDefault);
  }

  void ConfirmTarget(uint64_t aInputBlockId) {
    nsRefPtr<AsyncPanZoomController> target = this;
    GetInputQueue()->SetConfirmedTargetApzc(aInputBlockId, target);
  }

  void SetAllowedTouchBehavior(uint64_t aInputBlockId, const nsTArray<TouchBehaviorFlags>& aBehaviors) {
    GetInputQueue()->SetAllowedTouchBehavior(aInputBlockId, aBehaviors);
  }

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
                                      ParentLayerPoint& aScrollOffset) {
    bool ret = AdvanceAnimations(aSampleTime);
    AsyncPanZoomController::SampleContentTransformForFrame(
      aOutTransform, aScrollOffset);
    return ret;
  }

  void SetWaitForMainThread() {
    mWaitForMainThread = true;
  }

private:
  bool mWaitForMainThread;
};

AsyncPanZoomController*
TestAPZCTreeManager::MakeAPZCInstance(uint64_t aLayersId, GeckoContentController* aController)
{
  return new TestAsyncPanZoomController(aLayersId, aController, this, AsyncPanZoomController::USE_GESTURE_DETECTOR);
}

static FrameMetrics
TestFrameMetrics()
{
  FrameMetrics fm;

  fm.SetDisplayPort(CSSRect(0, 0, 10, 10));
  fm.mCompositionBounds = ParentLayerRect(0, 0, 10, 10);
  fm.SetCriticalDisplayPort(CSSRect(0, 0, 10, 10));
  fm.SetScrollableRect(CSSRect(0, 0, 100, 100));

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
    APZThreadUtils::SetThreadAssertionsEnabled(false);
    APZThreadUtils::SetControllerThread(MessageLoop::current());

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

  void MakeApzcWaitForMainThread()
  {
    apzc->SetWaitForMainThread();
  }

  void MakeApzcZoomable()
  {
    apzc->UpdateZoomConstraints(ZoomConstraints(true, true, CSSToParentLayerScale(0.25f), CSSToParentLayerScale(4.0f)));
  }

  void MakeApzcUnzoomable()
  {
    apzc->UpdateZoomConstraints(ZoomConstraints(false, false, CSSToParentLayerScale(1.0f), CSSToParentLayerScale(1.0f)));
  }

  void TestOverscroll();

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


















static SingleTouchData
CreateSingleTouchData(int32_t aIdentifier, int aX, int aY)
{
  SingleTouchData touch(aIdentifier, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0);
  touch.mLocalScreenPoint = ParentLayerPoint(aX, aY);
  return touch;
}
static PinchGestureInput
CreatePinchGestureInput(PinchGestureInput::PinchGestureType aType,
                        int aFocusX, int aFocusY,
                        float aCurrentSpan, float aPreviousSpan)
{
  PinchGestureInput result(aType, 0, TimeStamp(), ScreenPoint(aFocusX, aFocusY),
                           aCurrentSpan, aPreviousSpan, 0);
  result.mLocalFocusPoint = ParentLayerPoint(aFocusX, aFocusY);
  return result;
}

template<class InputReceiver> static void
SetDefaultAllowedTouchBehavior(const nsRefPtr<InputReceiver>& aTarget,
                               uint64_t aInputBlockId,
                               int touchPoints = 1)
{
  nsTArray<uint32_t> defaultBehaviors;
  
  for (int i = 0; i < touchPoints; i++) {
    defaultBehaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                                 | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN
                                 | mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM
                                 | mozilla::layers::AllowedTouchBehavior::DOUBLE_TAP_ZOOM);
  }
  aTarget->SetAllowedTouchBehavior(aInputBlockId, defaultBehaviors);
}

template<class InputReceiver> static nsEventStatus
TouchDown(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int aTime, uint64_t* aOutInputBlockId = nullptr)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(CreateSingleTouchData(0, aX, aY));
  return aTarget->ReceiveInputEvent(mti, nullptr, aOutInputBlockId);
}

template<class InputReceiver> static nsEventStatus
TouchMove(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int aTime)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(CreateSingleTouchData(0, aX, aY));
  return aTarget->ReceiveInputEvent(mti, nullptr, nullptr);
}

template<class InputReceiver> static nsEventStatus
TouchUp(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int aTime)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(CreateSingleTouchData(0, aX, aY));
  return aTarget->ReceiveInputEvent(mti, nullptr, nullptr);
}

template<class InputReceiver> static void
Tap(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int& aTime, int aTapLength,
    nsEventStatus (*aOutEventStatuses)[2] = nullptr,
    uint64_t* aOutInputBlockId = nullptr)
{
  
  
  uint64_t blockId;
  if (!aOutInputBlockId) {
    aOutInputBlockId = &blockId;
  }

  nsEventStatus status = TouchDown(aTarget, aX, aY, aTime, aOutInputBlockId);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }
  aTime += aTapLength;

  
  
  if (gfxPrefs::TouchActionEnabled() && status != nsEventStatus_eConsumeNoDefault) {
    SetDefaultAllowedTouchBehavior(aTarget, *aOutInputBlockId);
  }

  status = TouchUp(aTarget, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }
}

template<class InputReceiver> static void
TapAndCheckStatus(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int& aTime, int aTapLength)
{
  nsEventStatus statuses[2];
  Tap(aTarget, aX, aY, aTime, aTapLength, &statuses);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[0]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[1]);
}

template<class InputReceiver> static void
Pan(const nsRefPtr<InputReceiver>& aTarget,
    int& aTime,
    int aTouchStartY,
    int aTouchEndY,
    bool aKeepFingerDown = false,
    nsTArray<uint32_t>* aAllowedTouchBehaviors = nullptr,
    nsEventStatus (*aOutEventStatuses)[4] = nullptr,
    uint64_t* aOutInputBlockId = nullptr)
{
  
  
  
  
  gfxPrefs::SetAPZTouchStartTolerance(1.0f / 1000.0f);
  const int OVERCOME_TOUCH_TOLERANCE = 1;

  const int TIME_BETWEEN_TOUCH_EVENT = 100;

  
  
  uint64_t blockId;
  if (!aOutInputBlockId) {
    aOutInputBlockId = &blockId;
  }

  
  nsEventStatus status = TouchDown(aTarget, 10, aTouchStartY + OVERCOME_TOUCH_TOLERANCE, aTime, aOutInputBlockId);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  
  if (status != nsEventStatus_eConsumeNoDefault) {
    if (aAllowedTouchBehaviors) {
      EXPECT_EQ(1UL, aAllowedTouchBehaviors->Length());
      aTarget->SetAllowedTouchBehavior(*aOutInputBlockId, *aAllowedTouchBehaviors);
    } else if (gfxPrefs::TouchActionEnabled()) {
      SetDefaultAllowedTouchBehavior(aTarget, *aOutInputBlockId);
    }
  }

  status = TouchMove(aTarget, 10, aTouchStartY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  status = TouchMove(aTarget, 10, aTouchEndY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  if (!aKeepFingerDown) {
    status = TouchUp(aTarget, 10, aTouchEndY, aTime);
  } else {
    status = nsEventStatus_eIgnore;
  }
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;
}





template<class InputReceiver> static void
PanAndCheckStatus(const nsRefPtr<InputReceiver>& aTarget,
                  int& aTime,
                  int aTouchStartY,
                  int aTouchEndY,
                  bool aExpectConsumed,
                  nsTArray<uint32_t>* aAllowedTouchBehaviors,
                  uint64_t* aOutInputBlockId = nullptr)
{
  nsEventStatus statuses[4]; 
  Pan(aTarget, aTime, aTouchStartY, aTouchEndY, false, aAllowedTouchBehaviors, &statuses, aOutInputBlockId);

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
ApzcPanNoFling(const nsRefPtr<TestAsyncPanZoomController>& aApzc,
               int& aTime,
               int aTouchStartY,
               int aTouchEndY,
               uint64_t* aOutInputBlockId = nullptr)
{
  Pan(aApzc, aTime, aTouchStartY, aTouchEndY, false, nullptr, nullptr, aOutInputBlockId);
  aApzc->CancelAnimation();
}

template<class InputReceiver> static void
PinchWithPinchInput(const nsRefPtr<InputReceiver>& aTarget,
                    int aFocusX, int aFocusY, float aScale,
                    nsEventStatus (*aOutEventStatuses)[3] = nullptr)
{
  nsEventStatus actualStatus = aTarget->ReceiveInputEvent(
      CreatePinchGestureInput(PinchGestureInput::PINCHGESTURE_START,
                              aFocusX, aFocusY, 10.0, 10.0),
      nullptr);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = actualStatus;
  }
  actualStatus = aTarget->ReceiveInputEvent(
      CreatePinchGestureInput(PinchGestureInput::PINCHGESTURE_SCALE,
                              aFocusX, aFocusY, 10.0 * aScale, 10.0),
      nullptr);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = actualStatus;
  }
  actualStatus = aTarget->ReceiveInputEvent(
      CreatePinchGestureInput(PinchGestureInput::PINCHGESTURE_END,
                              
                              
                              aFocusX, aFocusY, -1.0, -1.0),
      nullptr);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = actualStatus;
  }
}

template<class InputReceiver> static void
PinchWithPinchInputAndCheckStatus(const nsRefPtr<InputReceiver>& aTarget,
                                  int aFocusX, int aFocusY, float aScale,
                                  bool aShouldTriggerPinch)
{
  nsEventStatus statuses[3];  
  PinchWithPinchInput(aTarget, aFocusX, aFocusY, aScale, &statuses);

  nsEventStatus expectedStatus = aShouldTriggerPinch
      ? nsEventStatus_eConsumeNoDefault
      : nsEventStatus_eIgnore;
  EXPECT_EQ(expectedStatus, statuses[0]);
  EXPECT_EQ(expectedStatus, statuses[1]);
}

template<class InputReceiver> static void
PinchWithTouchInput(const nsRefPtr<InputReceiver>& aTarget,
                    int aFocusX, int aFocusY, float aScale,
                    int& inputId,
                    nsTArray<uint32_t>* aAllowedTouchBehaviors = nullptr,
                    nsEventStatus (*aOutEventStatuses)[4] = nullptr,
                    uint64_t* aOutInputBlockId = nullptr)
{
  
  
  float pinchLength = 100.0;
  float pinchLengthScaled = pinchLength * aScale;

  
  
  uint64_t blockId;
  if (!aOutInputBlockId) {
    aOutInputBlockId = &blockId;
  }

  MultiTouchInput mtiStart = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, 0, TimeStamp(), 0);
  mtiStart.mTouches.AppendElement(CreateSingleTouchData(inputId, aFocusX, aFocusY));
  mtiStart.mTouches.AppendElement(CreateSingleTouchData(inputId + 1, aFocusX, aFocusY));
  nsEventStatus status = aTarget->ReceiveInputEvent(mtiStart, aOutInputBlockId);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }

  if (aAllowedTouchBehaviors) {
    EXPECT_EQ(2UL, aAllowedTouchBehaviors->Length());
    aTarget->SetAllowedTouchBehavior(*aOutInputBlockId, *aAllowedTouchBehaviors);
  } else if (gfxPrefs::TouchActionEnabled()) {
    SetDefaultAllowedTouchBehavior(aTarget, *aOutInputBlockId, 2);
  }

  MultiTouchInput mtiMove1 = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, 0, TimeStamp(), 0);
  mtiMove1.mTouches.AppendElement(CreateSingleTouchData(inputId, aFocusX - pinchLength, aFocusY));
  mtiMove1.mTouches.AppendElement(CreateSingleTouchData(inputId + 1, aFocusX + pinchLength, aFocusY));
  status = aTarget->ReceiveInputEvent(mtiMove1, nullptr);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }

  MultiTouchInput mtiMove2 = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, 0, TimeStamp(), 0);
  mtiMove2.mTouches.AppendElement(CreateSingleTouchData(inputId, aFocusX - pinchLengthScaled, aFocusY));
  mtiMove2.mTouches.AppendElement(CreateSingleTouchData(inputId + 1, aFocusX + pinchLengthScaled, aFocusY));
  status = aTarget->ReceiveInputEvent(mtiMove2, nullptr);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = status;
  }

  MultiTouchInput mtiEnd = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, 0, TimeStamp(), 0);
  mtiEnd.mTouches.AppendElement(CreateSingleTouchData(inputId, aFocusX - pinchLengthScaled, aFocusY));
  mtiEnd.mTouches.AppendElement(CreateSingleTouchData(inputId + 1, aFocusX + pinchLengthScaled, aFocusY));
  status = aTarget->ReceiveInputEvent(mtiEnd, nullptr);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }

  inputId += 2;
}

template<class InputReceiver> static void
PinchWithTouchInputAndCheckStatus(const nsRefPtr<InputReceiver>& aTarget,
                                  int aFocusX, int aFocusY, float aScale,
                                  int& inputId, bool aShouldTriggerPinch,
                                  nsTArray<uint32_t>* aAllowedTouchBehaviors)
{
  nsEventStatus statuses[4];  
  PinchWithTouchInput(aTarget, aFocusX, aFocusY, aScale, inputId, aAllowedTouchBehaviors, &statuses);

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
    fm.SetScrollableRect(CSSRect(0, 0, 980, 1000));
    fm.SetScrollOffset(CSSPoint(300, 300));
    fm.SetZoom(CSSToParentLayerScale2D(2.0, 2.0));
    
    fm.SetIsRoot(true);
    
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
      PinchWithTouchInputAndCheckStatus(apzc, 250, 300, 1.25, touchInputId, aShouldTriggerPinch, aAllowedTouchBehaviors);
    } else {
      PinchWithPinchInputAndCheckStatus(apzc, 250, 300, 1.25, aShouldTriggerPinch);
    }

    FrameMetrics fm = apzc->GetFrameMetrics();

    if (aShouldTriggerPinch) {
      
      EXPECT_EQ(2.5f, fm.GetZoom().ToScaleFactor().scale);
      EXPECT_EQ(305, fm.GetScrollOffset().x);
      EXPECT_EQ(310, fm.GetScrollOffset().y);
    } else {
      
      
      EXPECT_EQ(2.0f, fm.GetZoom().ToScaleFactor().scale);
      EXPECT_EQ(300, fm.GetScrollOffset().x);
      EXPECT_EQ(300, fm.GetScrollOffset().y);
    }

    
    
    fm.SetZoom(CSSToParentLayerScale2D(2.0, 2.0));
    fm.SetScrollOffset(CSSPoint(930, 5));
    apzc->SetFrameMetrics(fm);
    

    if (mGestureBehavior == AsyncPanZoomController::USE_GESTURE_DETECTOR) {
      PinchWithTouchInputAndCheckStatus(apzc, 250, 300, 0.5, touchInputId, aShouldTriggerPinch, aAllowedTouchBehaviors);
    } else {
      PinchWithPinchInputAndCheckStatus(apzc, 250, 300, 0.5, aShouldTriggerPinch);
    }

    fm = apzc->GetFrameMetrics();

    if (aShouldTriggerPinch) {
      
      EXPECT_EQ(1.0f, fm.GetZoom().ToScaleFactor().scale);
      EXPECT_EQ(880, fm.GetScrollOffset().x);
      EXPECT_EQ(0, fm.GetScrollOffset().y);
    } else {
      EXPECT_EQ(2.0f, fm.GetZoom().ToScaleFactor().scale);
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
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);
  DoPinchTest(true);
}

TEST_F(APZCPinchGestureDetectorTester, Pinch_UseGestureDetector_NoTouchAction) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);
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

  MakeApzcWaitForMainThread();
  MakeApzcZoomable();

  int touchInputId = 0;
  uint64_t blockId = 0;
  PinchWithTouchInput(apzc, 250, 300, 1.25, touchInputId, nullptr, nullptr, &blockId);

  
  apzc->ContentReceivedInputBlock(blockId, true);

  
  
  EXPECT_LE(1, mcc->RunThroughDelayedTasks());

  
  FrameMetrics fm = apzc->GetFrameMetrics();
  EXPECT_EQ(originalMetrics.GetZoom(), fm.GetZoom());
  EXPECT_EQ(originalMetrics.GetScrollOffset().x, fm.GetScrollOffset().x);
  EXPECT_EQ(originalMetrics.GetScrollOffset().y, fm.GetScrollOffset().y);

  apzc->AssertStateIsReset();
}

TEST_F(APZCBasicTester, Overzoom) {
  
  FrameMetrics fm;
  fm.mCompositionBounds = ParentLayerRect(0, 0, 100, 100);
  fm.SetScrollableRect(CSSRect(0, 0, 125, 150));
  fm.SetScrollOffset(CSSPoint(10, 0));
  fm.SetZoom(CSSToParentLayerScale2D(1.0, 1.0));
  fm.SetIsRoot(true);
  apzc->SetFrameMetrics(fm);

  MakeApzcZoomable();

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  PinchWithPinchInputAndCheckStatus(apzc, 50, 50, 0.5, true);

  fm = apzc->GetFrameMetrics();
  EXPECT_EQ(0.8f, fm.GetZoom().ToScaleFactor().scale);
  
  
  EXPECT_LT(abs(fm.GetScrollOffset().x), 1e-5);
  EXPECT_LT(abs(fm.GetScrollOffset().y), 1e-5);
}

TEST_F(APZCBasicTester, SimpleTransform) {
  ParentLayerPoint pointOut;
  ViewTransform viewTransformOut;
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

  EXPECT_EQ(ParentLayerPoint(), pointOut);
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
  transforms[0].PostScale(0.5f, 0.5f, 1.0f); 
  transforms[1].PostScale(2.0f, 1.0f, 1.0f); 

  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, lm, layers);

  FrameMetrics metrics;
  metrics.mCompositionBounds = ParentLayerRect(0, 0, 24, 24);
  metrics.SetDisplayPort(CSSRect(-1, -1, 6, 6));
  metrics.SetScrollOffset(CSSPoint(10, 10));
  metrics.SetScrollableRect(CSSRect(0, 0, 50, 50));
  metrics.SetCumulativeResolution(LayoutDeviceToLayerScale2D(2, 2));
  metrics.SetPresShellResolution(2.0f);
  metrics.SetZoom(CSSToParentLayerScale2D(6, 6));
  metrics.SetDevPixelsPerCSSPixel(CSSToLayoutDeviceScale(3));
  metrics.SetScrollId(FrameMetrics::START_SCROLL_ID);

  FrameMetrics childMetrics = metrics;
  childMetrics.SetScrollId(FrameMetrics::START_SCROLL_ID + 1);

  layers[0]->SetFrameMetrics(metrics);
  layers[1]->SetFrameMetrics(childMetrics);

  ParentLayerPoint pointOut;
  ViewTransform viewTransformOut;

  
  

  
  apzc->SetFrameMetrics(metrics);
  apzc->NotifyLayersUpdated(metrics, true);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerToParentLayerScale(1), ParentLayerPoint()), viewTransformOut);
  EXPECT_EQ(ParentLayerPoint(60, 60), pointOut);

  childApzc->SetFrameMetrics(childMetrics);
  childApzc->NotifyLayersUpdated(childMetrics, true);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerToParentLayerScale(1), ParentLayerPoint()), viewTransformOut);
  EXPECT_EQ(ParentLayerPoint(60, 60), pointOut);

  
  metrics.ScrollBy(CSSPoint(5, 0));
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerToParentLayerScale(1), ParentLayerPoint(-30, 0)), viewTransformOut);
  EXPECT_EQ(ParentLayerPoint(90, 60), pointOut);

  childMetrics.ScrollBy(CSSPoint(5, 0));
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerToParentLayerScale(1), ParentLayerPoint(-30, 0)), viewTransformOut);
  EXPECT_EQ(ParentLayerPoint(90, 60), pointOut);

  
  metrics.ZoomBy(1.5f);
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerToParentLayerScale(1.5), ParentLayerPoint(-45, 0)), viewTransformOut);
  EXPECT_EQ(ParentLayerPoint(135, 90), pointOut);

  childMetrics.ZoomBy(1.5f);
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerToParentLayerScale(1.5), ParentLayerPoint(-45, 0)), viewTransformOut);
  EXPECT_EQ(ParentLayerPoint(135, 90), pointOut);

  childApzc->Destroy();
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
    ParentLayerPoint pointOut;
    ViewTransform viewTransformOut;

    nsTArray<uint32_t> allowedTouchBehaviors;
    allowedTouchBehaviors.AppendElement(aBehavior);

    
    PanAndCheckStatus(apzc, time, touchStart, touchEnd, aShouldBeConsumed, &allowedTouchBehaviors);
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    if (aShouldTriggerScroll) {
      EXPECT_EQ(ParentLayerPoint(0, -(touchEnd-touchStart)), pointOut);
      EXPECT_NE(ViewTransform(), viewTransformOut);
    } else {
      EXPECT_EQ(ParentLayerPoint(), pointOut);
      EXPECT_EQ(ViewTransform(), viewTransformOut);
    }

    
    
    apzc->CancelAnimation();

    
    PanAndCheckStatus(apzc, time, touchEnd, touchStart, aShouldBeConsumed, &allowedTouchBehaviors);
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    EXPECT_EQ(ParentLayerPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);
  }

  void DoPanWithPreventDefaultTest()
  {
    MakeApzcWaitForMainThread();

    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;
    ParentLayerPoint pointOut;
    ViewTransform viewTransformOut;
    uint64_t blockId = 0;

    
    nsTArray<uint32_t> allowedTouchBehaviors;
    allowedTouchBehaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
    PanAndCheckStatus(apzc, time, touchStart, touchEnd, true, &allowedTouchBehaviors, &blockId);

    
    
    apzc->ContentReceivedInputBlock(blockId, true);
    
    
    EXPECT_LE(1, mcc->RunThroughDelayedTasks());

    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
    EXPECT_EQ(ParentLayerPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);

    apzc->AssertStateIsReset();
  }
};

TEST_F(APZCPanningTester, Pan) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);
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
  ParentLayerPoint pointOut;
  ViewTransform viewTransformOut;

  
  Pan(apzc, time, touchStart, touchEnd);
  ParentLayerPoint lastPoint;
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

  
  
  
  Pan(apzc, time, 25, 45);
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

TEST_F(APZCBasicTester, PanningTransformNotifications) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  
  
  
  
  
  
  

  MockFunction<void(std::string checkPointName)> check;
  {
    InSequence s;
    EXPECT_CALL(check, Call("Simple pan"));
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::StartTouch,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::TransformBegin,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::StartPanning,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::EndTouch,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::TransformEnd,_)).Times(1);
    EXPECT_CALL(check, Call("Complex pan"));
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::StartTouch,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::TransformBegin,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::StartPanning,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::EndTouch,_)).Times(1);
    EXPECT_CALL(*mcc, NotifyAPZStateChange(_,GeckoContentController::APZStateChange::TransformEnd,_)).Times(1);
    EXPECT_CALL(check, Call("Done"));
  }

  int time = 0;
  check.Call("Simple pan");
  ApzcPanNoFling(apzc, time, 50, 25);
  check.Call("Complex pan");
  Pan(apzc, time, 25, 45);
  apzc->AdvanceAnimationsUntilEnd(testStartTime);
  check.Call("Done");
}

void APZCBasicTester::TestOverscroll()
{
  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  Pan(apzc, time, touchStart, touchEnd);
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  const TimeDuration increment = TimeDuration::FromMilliseconds(1);
  bool recoveredFromOverscroll = false;
  ParentLayerPoint pointOut;
  ViewTransform viewTransformOut;
  while (apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut)) {
    
    EXPECT_EQ(ParentLayerPoint(0, 90), pointOut);

    
    
    apzc->GetOverscrollTransform();

    if (!apzc->IsOverscrolled()) {
      recoveredFromOverscroll = true;
    }

    testStartTime += increment;
  }
  EXPECT_TRUE(recoveredFromOverscroll);
  apzc->AssertStateIsReset();
}


TEST_F(APZCBasicTester, OverScrollPanning) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  TestOverscroll();
}



TEST_F(APZCBasicTester, OverScroll_Bug1152051) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  

  
  
  
  
  SCOPED_GFX_PREF(APZFlingFriction, float, 0);

  
  
  
  SCOPED_GFX_PREF(APZOverscrollSpringStiffness, float, 0.01225f);

  TestOverscroll();
}

TEST_F(APZCBasicTester, OverScrollAbort) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  Pan(apzc, time, touchStart, touchEnd);
  EXPECT_TRUE(apzc->IsOverscrolled());

  ParentLayerPoint pointOut;
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
  Pan(apzc, time, touchStart, touchEnd, true); 
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

    
    Pan(apzc, time, touchStart, touchEnd);
    
    while (mcc->RunThroughDelayedTasks());

    
    
    
    int timeDelta = aSlow ? 2000 : 10;
    int tapCallsExpected = aSlow ? 2 : 1;

    
    ParentLayerPoint pointOut;
    ViewTransform viewTransformOut;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(timeDelta), &viewTransformOut, pointOut);

    
    
    EXPECT_CALL(*mcc, HandleSingleTap(_, 0, apzc->GetGuid())).Times(tapCallsExpected);
    Tap(apzc, 10, 10, time, 0);
    while (mcc->RunThroughDelayedTasks());

    
    
    time += 500;
    Tap(apzc, 100, 100, time, 0);
    while (mcc->RunThroughDelayedTasks());

    
    ParentLayerPoint finalPointOut;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(timeDelta + 1000), &viewTransformOut, finalPointOut);
    EXPECT_EQ(pointOut.x, finalPointOut.x);
    EXPECT_EQ(pointOut.y, finalPointOut.y);

    apzc->AssertStateIsReset();
  }

  void DoFlingStopWithSlowListener(bool aPreventDefault) {
    MakeApzcWaitForMainThread();

    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;
    uint64_t blockId = 0;

    
    Pan(apzc, time, touchStart, touchEnd, false, nullptr, nullptr, &blockId);
    apzc->ContentReceivedInputBlock(blockId, false);
    while (mcc->RunThroughDelayedTasks());

    
    ParentLayerPoint point, finalPoint;
    ViewTransform viewTransform;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(10), &viewTransform, point);
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(20), &viewTransform, finalPoint);
    EXPECT_GT(finalPoint.y, point.y);

    
    TouchDown(apzc, 10, 10, time, &blockId);

    
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(30), &viewTransform, point);
    EXPECT_EQ(finalPoint.x, point.x);
    EXPECT_EQ(finalPoint.y, point.y);

    
    
    apzc->ContentReceivedInputBlock(blockId, aPreventDefault);
    while (mcc->RunThroughDelayedTasks());

    
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(100), &viewTransform, point);
    EXPECT_EQ(finalPoint.x, point.x);
    EXPECT_EQ(finalPoint.y, point.y);

    
    TouchUp(apzc, 10, 10, time);
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
  TapAndCheckStatus(apzc, 10, 10, time, 100);
  
  
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
  TapAndCheckStatus(apzc, 10, 10, time, 400);
  
  
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
    uint64_t blockId = 0;

    nsEventStatus status = TouchDown(apzc, 10, 10, time, &blockId);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    if (gfxPrefs::TouchActionEnabled() && status != nsEventStatus_eConsumeNoDefault) {
      
      nsTArray<uint32_t> allowedTouchBehaviors;
      allowedTouchBehaviors.AppendElement(aBehavior);
      apzc->SetAllowedTouchBehavior(blockId, allowedTouchBehaviors);
    }
    
    apzc->ContentReceivedInputBlock(blockId, false);

    MockFunction<void(std::string checkPointName)> check;

    {
      InSequence s;

      EXPECT_CALL(check, Call("preHandleLongTap"));
      blockId++;
      EXPECT_CALL(*mcc, HandleLongTap(CSSPoint(10, 10), 0, apzc->GetGuid(), blockId)).Times(1);
      EXPECT_CALL(check, Call("postHandleLongTap"));

      EXPECT_CALL(check, Call("preHandleSingleTap"));
      EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
      EXPECT_CALL(check, Call("postHandleSingleTap"));
    }

    
    mcc->CheckHasDelayedTask();

    
    check.Call("preHandleLongTap");
    mcc->RunDelayedTask();
    check.Call("postHandleLongTap");

    
    mcc->DestroyOldestTask();

    
    
    
    
    
    apzc->ContentReceivedInputBlock(blockId, false);
    mcc->CheckHasDelayedTask();
    mcc->RunDelayedTask();

    time += 1000;

    
    
    check.Call("preHandleSingleTap");
    status = TouchUp(apzc, 10, 10, time);
    mcc->RunDelayedTask();
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);
    check.Call("postHandleSingleTap");

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
    uint64_t blockId = 0;
    nsEventStatus status = TouchDown(apzc, touchX, touchStartY, time, &blockId);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    if (gfxPrefs::TouchActionEnabled() && status != nsEventStatus_eConsumeNoDefault) {
      
      nsTArray<uint32_t> allowedTouchBehaviors;
      allowedTouchBehaviors.AppendElement(aBehavior);
      apzc->SetAllowedTouchBehavior(blockId, allowedTouchBehaviors);
    }
    
    apzc->ContentReceivedInputBlock(blockId, false);

    MockFunction<void(std::string checkPointName)> check;

    {
      InSequence s;

      EXPECT_CALL(check, Call("preHandleLongTap"));
      blockId++;
      EXPECT_CALL(*mcc, HandleLongTap(CSSPoint(touchX, touchStartY), 0, apzc->GetGuid(), blockId)).Times(1);
      EXPECT_CALL(check, Call("postHandleLongTap"));
    }

    mcc->CheckHasDelayedTask();

    
    check.Call("preHandleLongTap");
    mcc->RunDelayedTask();
    check.Call("postHandleLongTap");

    
    mcc->DestroyOldestTask();

    
    
    
    
    
    apzc->ContentReceivedInputBlock(blockId, true);
    mcc->CheckHasDelayedTask();
    mcc->RunDelayedTask();

    time += 1000;

    MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, time, TimeStamp(), 0);
    mti.mTouches.AppendElement(SingleTouchData(0, ParentLayerPoint(touchX, touchEndY), ScreenSize(0, 0), 0, 0));
    status = apzc->ReceiveInputEvent(mti, nullptr);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(touchX, touchEndY), 0, apzc->GetGuid())).Times(0);
    status = TouchUp(apzc, touchX, touchEndY, time);
    EXPECT_EQ(nsEventStatus_eConsumeDoDefault, status);

    ParentLayerPoint pointOut;
    ViewTransform viewTransformOut;
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    EXPECT_EQ(ParentLayerPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);

    apzc->AssertStateIsReset();
  }
};

TEST_F(APZCLongPressTester, LongPress) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);
  DoLongPressTest(mozilla::layers::AllowedTouchBehavior::NONE);
}

TEST_F(APZCLongPressTester, LongPressWithTouchAction) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoLongPressTest(mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                  | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN
                  | mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
}

TEST_F(APZCLongPressTester, LongPressPreventDefault) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);
  DoLongPressPreventDefaultTest(mozilla::layers::AllowedTouchBehavior::NONE);
}

TEST_F(APZCLongPressTester, LongPressPreventDefaultWithTouchAction) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoLongPressPreventDefaultTest(mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                                | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN
                                | mozilla::layers::AllowedTouchBehavior::PINCH_ZOOM);
}

template<class InputReceiver> static void
DoubleTap(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int& aTime,
          nsEventStatus (*aOutEventStatuses)[4] = nullptr,
          uint64_t (*aOutInputBlockIds)[2] = nullptr)
{
  uint64_t blockId;
  nsEventStatus status = TouchDown(aTarget, aX, aY, aTime, &blockId);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[0] = status;
  }
  if (aOutInputBlockIds) {
    (*aOutInputBlockIds)[0] = blockId;
  }
  aTime += 10;

  
  
  if (gfxPrefs::TouchActionEnabled() && status != nsEventStatus_eConsumeNoDefault) {
    SetDefaultAllowedTouchBehavior(aTarget, blockId);
  }

  status = TouchUp(aTarget, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[1] = status;
  }
  aTime += 10;
  status = TouchDown(aTarget, aX, aY, aTime, &blockId);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[2] = status;
  }
  if (aOutInputBlockIds) {
    (*aOutInputBlockIds)[1] = blockId;
  }
  aTime += 10;

  if (gfxPrefs::TouchActionEnabled() && status != nsEventStatus_eConsumeNoDefault) {
    SetDefaultAllowedTouchBehavior(aTarget, blockId);
  }

  status = TouchUp(aTarget, aX, aY, aTime);
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }
}

template<class InputReceiver> static void
DoubleTapAndCheckStatus(const nsRefPtr<InputReceiver>& aTarget, int aX, int aY, int& aTime, uint64_t (*aOutInputBlockIds)[2] = nullptr)
{
  nsEventStatus statuses[4];
  DoubleTap(aTarget, aX, aY, aTime, &statuses, aOutInputBlockIds);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[0]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[1]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[2]);
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, statuses[3]);
}

TEST_F(APZCGestureDetectorTester, DoubleTap) {
  MakeApzcWaitForMainThread();
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);

  int time = 0;
  uint64_t blockIds[2];
  DoubleTapAndCheckStatus(apzc, 10, 10, time, &blockIds);

  
  apzc->ContentReceivedInputBlock(blockIds[0], false);
  apzc->ContentReceivedInputBlock(blockIds[1], false);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, DoubleTapNotZoomable) {
  MakeApzcWaitForMainThread();
  MakeApzcUnzoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(2);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);

  int time = 0;
  uint64_t blockIds[2];
  DoubleTapAndCheckStatus(apzc, 10, 10, time, &blockIds);

  
  apzc->ContentReceivedInputBlock(blockIds[0], false);
  apzc->ContentReceivedInputBlock(blockIds[1], false);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, DoubleTapPreventDefaultFirstOnly) {
  MakeApzcWaitForMainThread();
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);

  int time = 0;
  uint64_t blockIds[2];
  DoubleTapAndCheckStatus(apzc, 10, 10, time, &blockIds);

  
  apzc->ContentReceivedInputBlock(blockIds[0], true);
  apzc->ContentReceivedInputBlock(blockIds[1], false);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, DoubleTapPreventDefaultBoth) {
  MakeApzcWaitForMainThread();
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);
  EXPECT_CALL(*mcc, HandleDoubleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(0);

  int time = 0;
  uint64_t blockIds[2];
  DoubleTapAndCheckStatus(apzc, 10, 10, time, &blockIds);

  
  apzc->ContentReceivedInputBlock(blockIds[0], true);
  apzc->ContentReceivedInputBlock(blockIds[1], true);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}



TEST_F(APZCGestureDetectorTester, TapFollowedByPinch) {
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);

  int time = 0;
  Tap(apzc, 10, 10, time, 100);

  int inputId = 0;
  MultiTouchInput mti;
  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, time, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(inputId, ParentLayerPoint(20, 20), ScreenSize(0, 0), 0, 0));
  mti.mTouches.AppendElement(SingleTouchData(inputId + 1, ParentLayerPoint(10, 10), ScreenSize(0, 0), 0, 0));
  apzc->ReceiveInputEvent(mti, nullptr);

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, time, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(inputId, ParentLayerPoint(20, 20), ScreenSize(0, 0), 0, 0));
  mti.mTouches.AppendElement(SingleTouchData(inputId + 1, ParentLayerPoint(10, 10), ScreenSize(0, 0), 0, 0));
  apzc->ReceiveInputEvent(mti, nullptr);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, TapFollowedByMultipleTouches) {
  MakeApzcZoomable();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);

  int time = 0;
  Tap(apzc, 10, 10, time, 100);

  int inputId = 0;
  MultiTouchInput mti;
  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, time, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(inputId, ParentLayerPoint(20, 20), ScreenSize(0, 0), 0, 0));
  apzc->ReceiveInputEvent(mti, nullptr);

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, time, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(inputId, ParentLayerPoint(20, 20), ScreenSize(0, 0), 0, 0));
  mti.mTouches.AppendElement(SingleTouchData(inputId + 1, ParentLayerPoint(10, 10), ScreenSize(0, 0), 0, 0));
  apzc->ReceiveInputEvent(mti, nullptr);

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, time, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(inputId, ParentLayerPoint(20, 20), ScreenSize(0, 0), 0, 0));
  mti.mTouches.AppendElement(SingleTouchData(inputId + 1, ParentLayerPoint(10, 10), ScreenSize(0, 0), 0, 0));
  apzc->ReceiveInputEvent(mti, nullptr);

  while (mcc->RunThroughDelayedTasks());

  apzc->AssertStateIsReset();
}

class APZCTreeManagerTester : public ::testing::Test {
protected:
  virtual void SetUp() {
    gfxPrefs::GetSingleton();
    APZThreadUtils::SetThreadAssertionsEnabled(false);
    APZThreadUtils::SetControllerThread(MessageLoop::current());

    testStartTime = TimeStamp::Now();
    AsyncPanZoomController::SetFrameTime(testStartTime);

    mcc = new NiceMock<MockContentControllerDelayed>();
    manager = new TestAPZCTreeManager();
  }

  virtual void TearDown() {
    manager->ClearTree();
  }

  TimeStamp testStartTime;
  nsRefPtr<MockContentControllerDelayed> mcc;

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
    metrics.SetScrollableRect(aScrollableRect);
    metrics.SetScrollOffset(CSSPoint(0, 0));
    aLayer->SetFrameMetrics(metrics);
    aLayer->SetClipRect(Some(ViewAs<ParentLayerPixel>(layerBound)));
    if (!aScrollableRect.IsEqualEdges(CSSRect(-1, -1, -1, -1))) {
      
      
      
      EventRegions er = aLayer->GetEventRegions();
      nsIntRect scrollRect = LayerIntRect::ToUntyped(RoundedToInt(aScrollableRect * metrics.LayersPixelsPerCSSPixel()));
      er.mHitRegion = nsIntRegion(nsIntRect(layerBound.TopLeft(), scrollRect.Size()));
      aLayer->SetEventRegions(er);
    }
  }

  void SetScrollHandoff(Layer* aChild, Layer* aParent) {
    FrameMetrics metrics = aChild->GetFrameMetrics(0);
    metrics.SetScrollParentId(aParent->GetFrameMetrics(0).GetScrollId());
    aChild->SetFrameMetrics(metrics);
  }

  static TestAsyncPanZoomController* ApzcOf(Layer* aLayer) {
    EXPECT_EQ(1u, aLayer->GetFrameMetricsCount());
    return (TestAsyncPanZoomController*)aLayer->GetAsyncPanZoomController(0);
  }

  void CreateSimpleScrollingLayer() {
    const char* layerTreeSyntax = "t";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,200,200)),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 500, 500));
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

  void CreatePotentiallyLeakingTree() {
    const char* layerTreeSyntax = "c(c(c(t))c(c(t)))";
    
    root = CreateLayerTree(layerTreeSyntax, nullptr, nullptr, lm, layers);
    SetScrollableFrameMetrics(layers[0], FrameMetrics::START_SCROLL_ID);
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[5], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2);
    SetScrollableFrameMetrics(layers[6], FrameMetrics::START_SCROLL_ID + 3);
  }
};

class APZHitTestingTester : public APZCTreeManagerTester {
protected:
  Matrix4x4 transformToApzc;
  Matrix4x4 transformToGecko;

  already_AddRefed<AsyncPanZoomController> GetTargetAPZC(const ScreenPoint& aPoint) {
    nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(aPoint, nullptr);
    if (hit) {
      transformToApzc = manager->GetScreenToApzcTransform(hit.get());
      transformToGecko = manager->GetApzcToGeckoTransform(hit.get());
    }
    return hit.forget();
  }

protected:
  void CreateHitTesting1LayerTree() {
    const char* layerTreeSyntax = "c(tttt)";
    
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
    const char* layerTreeSyntax = "c(tc(t))";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,100,100)),
      nsIntRegion(nsIntRect(10,10,40,40)),
      nsIntRegion(nsIntRect(10,60,40,40)),
      nsIntRegion(nsIntRect(10,60,40,40)),
    };
    Matrix4x4 transforms[] = {
      Matrix4x4(),
      Matrix4x4(),
      Matrix4x4::Scaling(2, 1, 1),
      Matrix4x4(),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, lm, layers);

    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 80, 80));
    SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 0, 80, 80));
  }

  void CreateComplexMultiLayerTree() {
    const char* layerTreeSyntax = "c(tc(t)tc(c(t)tt))";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,300,400)),      
      nsIntRegion(nsIntRect(0,0,100,100)),      
      nsIntRegion(nsIntRect(50,50,200,300)),    
      nsIntRegion(nsIntRect(50,50,200,300)),    
      nsIntRegion(nsIntRect(0,200,100,100)),    
      nsIntRegion(nsIntRect(200,0,100,400)),    
      nsIntRegion(nsIntRect(200,0,100,200)),    
      nsIntRegion(nsIntRect(200,0,100,200)),    
      nsIntRegion(nsIntRect(200,200,100,100)),  
      nsIntRegion(nsIntRect(200,300,100,100)),  
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID);
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID);
    SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[6], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[7], FrameMetrics::START_SCROLL_ID + 2);
    SetScrollableFrameMetrics(layers[8], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[9], FrameMetrics::START_SCROLL_ID + 3);
  }

  void CreateBug1148350LayerTree() {
    const char* layerTreeSyntax = "c(t)";
    
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0,0,200,200)),
      nsIntRegion(nsIntRect(0,0,200,200)),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID);
  }
};


TEST_F(APZHitTestingTester, HitTesting1) {
  CreateHitTesting1LayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  
  nsRefPtr<AsyncPanZoomController> hit = GetTargetAPZC(ScreenPoint(20, 20));
  TestAsyncPanZoomController* nullAPZC = nullptr;
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(Matrix4x4(), transformToApzc);
  EXPECT_EQ(Matrix4x4(), transformToGecko);

  uint32_t paintSequenceNumber = 0;

  
  SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, paintSequenceNumber++);
  hit = GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(ApzcOf(root), hit.get());
  
  EXPECT_EQ(Point(15, 15), transformToApzc * Point(15, 15));
  EXPECT_EQ(Point(15, 15), transformToGecko * Point(15, 15));

  
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, paintSequenceNumber++);
  EXPECT_NE(ApzcOf(root), ApzcOf(layers[3]));
  hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(ApzcOf(layers[3]), hit.get());
  
  EXPECT_EQ(Point(25, 25), transformToApzc * Point(25, 25));
  EXPECT_EQ(Point(25, 25), transformToGecko * Point(25, 25));

  
  
  hit = GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(ApzcOf(root), hit.get());

  
  SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 2);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, paintSequenceNumber++);
  hit = GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(ApzcOf(layers[4]), hit.get());
  
  EXPECT_EQ(Point(15, 15), transformToApzc * Point(15, 15));
  EXPECT_EQ(Point(15, 15), transformToGecko * Point(15, 15));

  
  hit = GetTargetAPZC(ScreenPoint(90, 90));
  EXPECT_EQ(ApzcOf(root), hit.get());
  
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

  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);

  
  
  
  
  

  TestAsyncPanZoomController* apzcroot = ApzcOf(root);
  TestAsyncPanZoomController* apzc1 = ApzcOf(layers[1]);
  TestAsyncPanZoomController* apzc3 = ApzcOf(layers[3]);

  
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

TEST_F(APZCTreeManagerTester, ScrollablePaintedLayers) {
  CreateSimpleMultiLayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  
  SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID);
  SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);

  TestAsyncPanZoomController* nullAPZC = nullptr;
  
  EXPECT_FALSE(layers[0]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, ApzcOf(layers[1]));
  EXPECT_NE(nullAPZC, ApzcOf(layers[2]));
  EXPECT_EQ(ApzcOf(layers[1]), ApzcOf(layers[2]));

  
  SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  EXPECT_NE(ApzcOf(layers[1]), ApzcOf(layers[2]));

  
  
  SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  EXPECT_EQ(ApzcOf(layers[1]), ApzcOf(layers[2]));
}

TEST_F(APZCTreeManagerTester, Bug1068268) {
  CreatePotentiallyLeakingTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  nsRefPtr<HitTestingTreeNode> root = manager->GetRootNode();
  nsRefPtr<HitTestingTreeNode> node2 = root->GetFirstChild()->GetFirstChild();
  nsRefPtr<HitTestingTreeNode> node5 = root->GetLastChild()->GetLastChild();

  EXPECT_EQ(ApzcOf(layers[2]), node5->GetApzc());
  EXPECT_EQ(ApzcOf(layers[2]), node2->GetApzc());
  EXPECT_EQ(ApzcOf(layers[0]), ApzcOf(layers[2])->GetParent());
  EXPECT_EQ(ApzcOf(layers[2]), ApzcOf(layers[5]));

  EXPECT_EQ(node2->GetFirstChild(), node2->GetLastChild());
  EXPECT_EQ(ApzcOf(layers[3]), node2->GetLastChild()->GetApzc());
  EXPECT_EQ(node5->GetFirstChild(), node5->GetLastChild());
  EXPECT_EQ(ApzcOf(layers[6]), node5->GetLastChild()->GetApzc());
  EXPECT_EQ(ApzcOf(layers[2]), ApzcOf(layers[3])->GetParent());
  EXPECT_EQ(ApzcOf(layers[5]), ApzcOf(layers[6])->GetParent());
}

TEST_F(APZHitTestingTester, ComplexMultiLayerTree) {
  CreateComplexMultiLayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);

  















  TestAsyncPanZoomController* nullAPZC = nullptr;
  
  EXPECT_FALSE(layers[0]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, ApzcOf(layers[1]));
  EXPECT_NE(nullAPZC, ApzcOf(layers[2]));
  EXPECT_FALSE(layers[3]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, ApzcOf(layers[4]));
  EXPECT_FALSE(layers[5]->HasScrollableFrameMetrics());
  EXPECT_NE(nullAPZC, ApzcOf(layers[6]));
  EXPECT_NE(nullAPZC, ApzcOf(layers[7]));
  EXPECT_NE(nullAPZC, ApzcOf(layers[8]));
  EXPECT_NE(nullAPZC, ApzcOf(layers[9]));
  
  EXPECT_EQ(ApzcOf(layers[1]), ApzcOf(layers[2]));
  EXPECT_EQ(ApzcOf(layers[4]), ApzcOf(layers[6]));
  EXPECT_EQ(ApzcOf(layers[8]), ApzcOf(layers[6]));
  
  EXPECT_NE(ApzcOf(layers[1]), ApzcOf(layers[4]));
  EXPECT_NE(ApzcOf(layers[1]), ApzcOf(layers[7]));
  EXPECT_NE(ApzcOf(layers[1]), ApzcOf(layers[9]));
  EXPECT_NE(ApzcOf(layers[4]), ApzcOf(layers[7]));
  EXPECT_NE(ApzcOf(layers[4]), ApzcOf(layers[9]));
  EXPECT_NE(ApzcOf(layers[7]), ApzcOf(layers[9]));
  
  TestAsyncPanZoomController* layers1_2 = ApzcOf(layers[1]);
  TestAsyncPanZoomController* layers4_6_8 = ApzcOf(layers[4]);
  TestAsyncPanZoomController* layer7 = ApzcOf(layers[7]);
  TestAsyncPanZoomController* layer9 = ApzcOf(layers[9]);
  EXPECT_EQ(nullptr, layers1_2->GetParent());
  EXPECT_EQ(nullptr, layers4_6_8->GetParent());
  EXPECT_EQ(layers4_6_8, layer7->GetParent());
  EXPECT_EQ(nullptr, layer9->GetParent());
  
  nsRefPtr<HitTestingTreeNode> root = manager->GetRootNode();
  nsRefPtr<HitTestingTreeNode> node5 = root->GetLastChild();
  nsRefPtr<HitTestingTreeNode> node4 = node5->GetPrevSibling();
  nsRefPtr<HitTestingTreeNode> node2 = node4->GetPrevSibling();
  nsRefPtr<HitTestingTreeNode> node1 = node2->GetPrevSibling();
  nsRefPtr<HitTestingTreeNode> node3 = node2->GetLastChild();
  nsRefPtr<HitTestingTreeNode> node9 = node5->GetLastChild();
  nsRefPtr<HitTestingTreeNode> node8 = node9->GetPrevSibling();
  nsRefPtr<HitTestingTreeNode> node6 = node8->GetPrevSibling();
  nsRefPtr<HitTestingTreeNode> node7 = node6->GetLastChild();
  EXPECT_EQ(nullptr, node1->GetPrevSibling());
  EXPECT_EQ(nullptr, node3->GetPrevSibling());
  EXPECT_EQ(nullptr, node6->GetPrevSibling());
  EXPECT_EQ(nullptr, node7->GetPrevSibling());
  EXPECT_EQ(nullptr, node1->GetLastChild());
  EXPECT_EQ(nullptr, node3->GetLastChild());
  EXPECT_EQ(nullptr, node4->GetLastChild());
  EXPECT_EQ(nullptr, node7->GetLastChild());
  EXPECT_EQ(nullptr, node8->GetLastChild());
  EXPECT_EQ(nullptr, node9->GetLastChild());

  nsRefPtr<AsyncPanZoomController> hit = GetTargetAPZC(ScreenPoint(25, 25));
  EXPECT_EQ(ApzcOf(layers[1]), hit.get());
  hit = GetTargetAPZC(ScreenPoint(275, 375));
  EXPECT_EQ(ApzcOf(layers[9]), hit.get());
  hit = GetTargetAPZC(ScreenPoint(250, 100));
  EXPECT_EQ(ApzcOf(layers[7]), hit.get());
}

TEST_F(APZHitTestingTester, TestRepaintFlushOnNewInputBlock) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, false);

  
  
  
  

  CreateSimpleScrollingLayer();
  ScopedLayerTreeRegistration registration(0, root, mcc);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  TestAsyncPanZoomController* apzcroot = ApzcOf(root);

  
  

  MockFunction<void(std::string checkPointName)> check;

  {
    InSequence s;

    EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(AtLeast(1));
    EXPECT_CALL(check, Call("post-first-touch-start"));
    EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(AtLeast(1));
    EXPECT_CALL(check, Call("post-second-fling"));
    EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(AtLeast(1));
    EXPECT_CALL(check, Call("post-second-touch-start"));
  }

  int time = 0;
  
  ApzcPanNoFling(apzcroot, time, 100, 50);

  
  ScreenIntPoint touchPoint(50, 50);
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, time, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, touchPoint, ScreenSize(0, 0), 0, 0));

  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, manager->ReceiveInputEvent(mti, nullptr, nullptr));
  EXPECT_EQ(touchPoint, mti.mTouches[0].mScreenPoint);
  check.Call("post-first-touch-start");

  
  mti.mType = MultiTouchInput::MULTITOUCH_END;
  manager->ReceiveInputEvent(mti, nullptr, nullptr);

  AsyncPanZoomController::SetFrameTime(testStartTime + TimeDuration::FromMilliseconds(1000));

  
  
  
  
  
  
  ApzcPanNoFling(apzcroot, time, 100, 50);
  check.Call("post-second-fling");
  ApzcPanNoFling(apzcroot, time, 100, 50);

  
  
  mti.mType = MultiTouchInput::MULTITOUCH_START;
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, manager->ReceiveInputEvent(mti, nullptr, nullptr));
  EXPECT_EQ(touchPoint, mti.mTouches[0].mScreenPoint);
  check.Call("post-second-touch-start");

  mti.mType = MultiTouchInput::MULTITOUCH_END;
  EXPECT_EQ(nsEventStatus_eConsumeDoDefault, manager->ReceiveInputEvent(mti, nullptr, nullptr));
  EXPECT_EQ(touchPoint, mti.mTouches[0].mScreenPoint);

  mcc->RunThroughDelayedTasks();
}

TEST_F(APZHitTestingTester, Bug1148350) {
  CreateBug1148350LayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);

  MockFunction<void(std::string checkPointName)> check;
  {
    InSequence s;
    EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(100, 100), 0, ApzcOf(layers[1])->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped without transform"));
    EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(100, 100), 0, ApzcOf(layers[1])->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped with interleaved transform"));
  }

  int time = 0;
  Tap(manager, 100, 100, time, 100);
  mcc->RunThroughDelayedTasks();
  check.Call("Tapped without transform");

  uint64_t blockId;
  TouchDown(manager, 100, 100, time, &blockId);
  if (gfxPrefs::TouchActionEnabled()) {
    SetDefaultAllowedTouchBehavior(manager, blockId);
  }
  time += 100;

  layers[0]->SetVisibleRegion(nsIntRegion(nsIntRect(0,50,200,150)));
  layers[0]->SetBaseTransform(Matrix4x4::Translation(0, 50, 0));
  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);

  TouchUp(manager, 100, 100, time);
  mcc->RunThroughDelayedTasks();
  check.Call("Tapped with interleaved transform");
}

class APZOverscrollHandoffTester : public APZCTreeManagerTester {
protected:
  UniquePtr<ScopedLayerTreeRegistration> registration;
  TestAsyncPanZoomController* rootApzc;

  void CreateOverscrollHandoffLayerTree1() {
    const char* layerTreeSyntax = "c(t)";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 50, 100, 50))
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 100, 100));
    SetScrollHandoff(layers[1], root);
    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
    rootApzc = ApzcOf(root);
  }

  void CreateOverscrollHandoffLayerTree2() {
    const char* layerTreeSyntax = "c(c(t))";
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
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
    rootApzc = ApzcOf(root);
  }

  void CreateOverscrollHandoffLayerTree3() {
    const char* layerTreeSyntax = "c(c(t)c(t))";
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
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  }

  void CreateScrollgrabLayerTree() {
    const char* layerTreeSyntax = "c(t)";
    nsIntRegion layerVisibleRegion[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),  
      nsIntRegion(nsIntRect(0, 20, 100, 80))   
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 100, 120));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 100, 200));
    SetScrollHandoff(layers[1], root);
    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
    rootApzc = ApzcOf(root);
    rootApzc->GetFrameMetrics().SetHasScrollgrab(true);
  }
};




TEST_F(APZOverscrollHandoffTester, DeferredInputEventProcessing) {
  
  CreateOverscrollHandoffLayerTree1();

  TestAsyncPanZoomController* childApzc = ApzcOf(layers[1]);

  
  
  childApzc->SetWaitForMainThread();

  
  int time = 0;
  uint64_t blockId = 0;
  ApzcPanNoFling(childApzc, time, 90, 30, &blockId);

  
  childApzc->ContentReceivedInputBlock(blockId, false);
  childApzc->ConfirmTarget(blockId);

  
  EXPECT_EQ(50, childApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(10, rootApzc->GetFrameMetrics().GetScrollOffset().y);
}






TEST_F(APZOverscrollHandoffTester, LayerStructureChangesWhileEventsArePending) {
  
  CreateOverscrollHandoffLayerTree1();

  TestAsyncPanZoomController* childApzc = ApzcOf(layers[1]);

  
  
  childApzc->SetWaitForMainThread();

  
  int time = 0;
  uint64_t blockId = 0;
  ApzcPanNoFling(childApzc, time, 90, 30, &blockId);

  
  
  CreateOverscrollHandoffLayerTree2();
  nsRefPtr<Layer> middle = layers[1];
  childApzc->SetWaitForMainThread();
  TestAsyncPanZoomController* middleApzc = ApzcOf(middle);

  
  uint64_t secondBlockId = 0;
  ApzcPanNoFling(childApzc, time, 30, 90, &secondBlockId);

  
  childApzc->ContentReceivedInputBlock(blockId, false);
  childApzc->ConfirmTarget(blockId);

  
  
  EXPECT_EQ(50, childApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(10, rootApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(0, middleApzc->GetFrameMetrics().GetScrollOffset().y);

  
  childApzc->ContentReceivedInputBlock(secondBlockId, false);
  childApzc->ConfirmTarget(secondBlockId);

  
  
  EXPECT_EQ(0, childApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(10, rootApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(-10, middleApzc->GetFrameMetrics().GetScrollOffset().y);
}



TEST_F(APZOverscrollHandoffTester, StuckInOverscroll_Bug1073250) {
  
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  CreateOverscrollHandoffLayerTree1();

  TestAsyncPanZoomController* child = ApzcOf(layers[1]);

  
  int time = 0;
  Pan(manager, time, 10, 40, true );
  EXPECT_FALSE(child->IsOverscrolled());
  EXPECT_TRUE(rootApzc->IsOverscrolled());

  
  MultiTouchInput secondFingerDown(MultiTouchInput::MULTITOUCH_START, 0, TimeStamp(), 0);
  
  secondFingerDown.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, 40), ScreenSize(0, 0), 0, 0));
  secondFingerDown.mTouches.AppendElement(SingleTouchData(1, ScreenIntPoint(30, 20), ScreenSize(0, 0), 0, 0));
  manager->ReceiveInputEvent(secondFingerDown, nullptr, nullptr);

  
  MultiTouchInput fingersUp = secondFingerDown;
  fingersUp.mType = MultiTouchInput::MULTITOUCH_END;
  manager->ReceiveInputEvent(fingersUp, nullptr, nullptr);

  
  child->AdvanceAnimationsUntilEnd(testStartTime);
  rootApzc->AdvanceAnimationsUntilEnd(testStartTime);

  
  EXPECT_FALSE(child->IsOverscrolled());
  EXPECT_FALSE(rootApzc->IsOverscrolled());
}



TEST_F(APZOverscrollHandoffTester, SimultaneousFlings) {
  
  CreateOverscrollHandoffLayerTree3();

  nsRefPtr<TestAsyncPanZoomController> parent1 = ApzcOf(layers[1]);
  nsRefPtr<TestAsyncPanZoomController> child1 = ApzcOf(layers[2]);
  nsRefPtr<TestAsyncPanZoomController> parent2 = ApzcOf(layers[3]);
  nsRefPtr<TestAsyncPanZoomController> child2 = ApzcOf(layers[4]);

  
  int time = 0;
  Pan(child2, time, 45, 5);

  
  Pan(child1, time, 95, 55);

  
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

  nsRefPtr<TestAsyncPanZoomController> childApzc = ApzcOf(layers[1]);

  
  
  int time = 0;
  Pan(childApzc, time, 80, 45);

  
  EXPECT_EQ(20, rootApzc->GetFrameMetrics().GetScrollOffset().y);
  EXPECT_EQ(15, childApzc->GetFrameMetrics().GetScrollOffset().y);
}

TEST_F(APZOverscrollHandoffTester, ScrollgrabFling) {
  
  CreateScrollgrabLayerTree();

  nsRefPtr<TestAsyncPanZoomController> childApzc = ApzcOf(layers[1]);

  
  int time = 0;
  Pan(childApzc, time, 80, 70);

  
  rootApzc->AssertStateIsFling();
  childApzc->AssertStateIsReset();
}

class APZEventRegionsTester : public APZCTreeManagerTester {
protected:
  UniquePtr<ScopedLayerTreeRegistration> registration;
  TestAsyncPanZoomController* rootApzc;

  void CreateEventRegionsLayerTree1() {
    const char* layerTreeSyntax = "c(tt)";
    nsIntRegion layerVisibleRegions[] = {
      nsIntRegion(nsIntRect(0, 0, 200, 200)),     
      nsIntRegion(nsIntRect(0, 0, 100, 200)),     
      nsIntRegion(nsIntRect(0, 100, 200, 100)),   
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegions, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID);
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1);
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 2);
    SetScrollHandoff(layers[1], root);
    SetScrollHandoff(layers[2], root);

    
    
    
    
    
    
    EventRegions regions(nsIntRegion(nsIntRect(0, 0, 200, 200)));
    root->SetEventRegions(regions);
    regions.mDispatchToContentHitRegion = nsIntRegion(nsIntRect(0, 100, 100, 100));
    regions.mHitRegion = nsIntRegion(nsIntRect(0, 0, 100, 200));
    layers[1]->SetEventRegions(regions);
    regions.mHitRegion = nsIntRegion(nsIntRect(0, 100, 200, 100));
    layers[2]->SetEventRegions(regions);

    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
    rootApzc = ApzcOf(root);
  }

  void CreateEventRegionsLayerTree2() {
    const char* layerTreeSyntax = "c(t)";
    nsIntRegion layerVisibleRegions[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 500)),
      nsIntRegion(nsIntRect(0, 150, 100, 100)),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegions, nullptr, lm, layers);
    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID);

    
    
    EventRegions regions(nsIntRegion(nsIntRect(0, 0, 100, 100)));
    root->SetEventRegions(regions);
    regions.mHitRegion = nsIntRegion(nsIntRect(0, 150, 100, 100));
    layers[1]->SetEventRegions(regions);

    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
    rootApzc = ApzcOf(root);
  }

  void CreateObscuringLayerTree() {
    const char* layerTreeSyntax = "c(c(t)t)";
    
    
    
    
    
    nsIntRegion layerVisibleRegions[] = {
        
        nsIntRegion(nsIntRect(0,   0, 200, 200)),  
        nsIntRegion(nsIntRect(0,   0, 200, 200)),  
        nsIntRegion(nsIntRect(0, 100, 200,  50)),  
        nsIntRegion(nsIntRect(0, 100, 200, 100))   
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegions, nullptr, lm, layers);

    SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 200, 300));
    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 0, 200, 100));
    SetScrollHandoff(layers[2], layers[1]);
    SetScrollHandoff(layers[1], root);

    EventRegions regions(nsIntRegion(nsIntRect(0, 0, 200, 200)));
    root->SetEventRegions(regions);
    regions.mHitRegion = nsIntRegion(nsIntRect(0, 0, 200, 300));
    layers[1]->SetEventRegions(regions);
    regions.mHitRegion = nsIntRegion(nsIntRect(0, 100, 200, 100));
    layers[2]->SetEventRegions(regions);

    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
    rootApzc = ApzcOf(root);
  }

  void CreateBug1119497LayerTree() {
    const char* layerTreeSyntax = "c(tt)";
    
    
    
    
    nsIntRegion layerVisibleRegions[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegions, nullptr, lm, layers);

    SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1);

    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  }

  void CreateBug1117712LayerTree() {
    const char* layerTreeSyntax = "c(c(t)t)";
    
    
    
    
    
    
    
    nsIntRegion layerVisibleRegions[] = {
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
      nsIntRegion(nsIntRect(0, 0, 0, 0)),
      nsIntRegion(nsIntRect(0, 0, 10, 10)),
      nsIntRegion(nsIntRect(0, 0, 100, 100)),
    };
    Matrix4x4 layerTransforms[] = {
      Matrix4x4(),
      Matrix4x4::Translation(50, 0, 0),
      Matrix4x4(),
      Matrix4x4(),
    };
    root = CreateLayerTree(layerTreeSyntax, layerVisibleRegions, layerTransforms, lm, layers);

    SetScrollableFrameMetrics(layers[2], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 10, 10));
    SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 0, 100, 100));

    EventRegions regions(nsIntRegion(nsIntRect(0, 0, 10, 10)));
    layers[2]->SetEventRegions(regions);
    regions.mHitRegion = nsIntRegion(nsIntRect(0, 0, 100, 100));
    regions.mDispatchToContentHitRegion = nsIntRegion(nsIntRect(0, 0, 100, 100));
    layers[3]->SetEventRegions(regions);

    registration = MakeUnique<ScopedLayerTreeRegistration>(0, root, mcc);
    manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);
  }
};

TEST_F(APZEventRegionsTester, HitRegionImmediateResponse) {
  CreateEventRegionsLayerTree1();

  TestAsyncPanZoomController* root = ApzcOf(layers[0]);
  TestAsyncPanZoomController* left = ApzcOf(layers[1]);
  TestAsyncPanZoomController* bottom = ApzcOf(layers[2]);

  MockFunction<void(std::string checkPointName)> check;
  {
    InSequence s;
    EXPECT_CALL(*mcc, HandleSingleTap(_, _, left->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped on left"));
    EXPECT_CALL(*mcc, HandleSingleTap(_, _, bottom->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped on bottom"));
    EXPECT_CALL(*mcc, HandleSingleTap(_, _, root->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped on root"));
    EXPECT_CALL(check, Call("Tap pending on d-t-c region"));
    EXPECT_CALL(*mcc, HandleSingleTap(_, _, bottom->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped on bottom again"));
    EXPECT_CALL(*mcc, HandleSingleTap(_, _, left->GetGuid())).Times(1);
    EXPECT_CALL(check, Call("Tapped on left this time"));
  }

  int time = 0;
  
  
  Tap(manager, 10, 10, time, 100);
  mcc->RunThroughDelayedTasks();    
  check.Call("Tapped on left");
  Tap(manager, 110, 110, time, 100);
  mcc->RunThroughDelayedTasks();    
  check.Call("Tapped on bottom");
  Tap(manager, 110, 10, time, 100);
  mcc->RunThroughDelayedTasks();    
  check.Call("Tapped on root");

  
  Tap(manager, 10, 110, time, 100);
  mcc->RunThroughDelayedTasks();    
  check.Call("Tap pending on d-t-c region");
  mcc->RunThroughDelayedTasks();    
  check.Call("Tapped on bottom again");

  
  uint64_t inputBlockId = 0;
  Tap(manager, 10, 110, time, 100, nullptr, &inputBlockId);
  nsTArray<ScrollableLayerGuid> targets;
  targets.AppendElement(left->GetGuid());
  manager->SetTargetAPZC(inputBlockId, targets);
  while (mcc->RunThroughDelayedTasks());    
  check.Call("Tapped on left this time");
}

TEST_F(APZEventRegionsTester, HitRegionAccumulatesChildren) {
  CreateEventRegionsLayerTree2();

  int time = 0;
  
  
  
  
  EXPECT_CALL(*mcc, HandleSingleTap(_, _, rootApzc->GetGuid())).Times(1);
  Tap(manager, 10, 160, time, 100);
  mcc->RunThroughDelayedTasks();    
}

TEST_F(APZEventRegionsTester, Obscuration) {
  CreateObscuringLayerTree();
  ScopedLayerTreeRegistration registration(0, root, mcc);

  manager->UpdateHitTestingTree(nullptr, root, false, 0, 0);

  TestAsyncPanZoomController* parent = ApzcOf(layers[1]);
  TestAsyncPanZoomController* child = ApzcOf(layers[2]);

  int time = 0;
  ApzcPanNoFling(parent, time, 75, 25);

  HitTestResult result;
  nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(ScreenPoint(50, 75), &result);
  EXPECT_EQ(child, hit.get());
  EXPECT_EQ(HitTestResult::HitLayer, result);
}

TEST_F(APZEventRegionsTester, Bug1119497) {
  CreateBug1119497LayerTree();

  HitTestResult result;
  nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(ScreenPoint(50, 50), &result);
  
  
  EXPECT_EQ(nullptr, hit.get());
  EXPECT_EQ(HitTestResult::HitLayer, result);
}

TEST_F(APZEventRegionsTester, Bug1117712) {
  CreateBug1117712LayerTree();

  TestAsyncPanZoomController* apzc2 = ApzcOf(layers[2]);

  
  
  int time = 0;
  uint64_t inputBlockId = 0;
  Tap(manager, 55, 5, time, 100, nullptr, &inputBlockId);
  
  
  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(55, 5), 0, apzc2->GetGuid())).Times(1);

  nsTArray<ScrollableLayerGuid> targets;
  targets.AppendElement(apzc2->GetGuid());
  manager->SetTargetAPZC(inputBlockId, targets);
  while (mcc->RunThroughDelayedTasks());    
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
