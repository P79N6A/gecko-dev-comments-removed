




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/Attributes.h"
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCTreeManager.h"
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

class APZCTreeManagerTester : public ::testing::Test {
protected:
  virtual void SetUp() {
    gfxPrefs::GetSingleton();
    AsyncPanZoomController::SetThreadAssertionsEnabled(false);
  }
};

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

  FrameMetrics GetFrameMetrics() {
    ReentrantMonitorAutoEnter lock(mMonitor);
    return mFrameMetrics;
  }

  void AssertStateIsReset() {
    ReentrantMonitorAutoEnter lock(mMonitor);
    EXPECT_EQ(NOTHING, mState);
  }
};

class TestAPZCTreeManager : public APZCTreeManager {
public:
  
  void BuildOverscrollHandoffChain(AsyncPanZoomController* aApzc) {
    APZCTreeManager::BuildOverscrollHandoffChain(aApzc);
  }
  void ClearOverscrollHandoffChain() {
    APZCTreeManager::ClearOverscrollHandoffChain();
  }
};

static FrameMetrics
TestFrameMetrics()
{
  FrameMetrics fm;

  fm.mDisplayPort = CSSRect(0, 0, 10, 10);
  fm.mCompositionBounds = ParentLayerRect(0, 0, 10, 10);
  fm.mCriticalDisplayPort = CSSRect(0, 0, 10, 10);
  fm.mScrollableRect = CSSRect(0, 0, 100, 100);
  fm.mViewport = CSSRect(0, 0, 10, 10);

  return fm;
}

class APZCBasicTester : public ::testing::Test {
public:
  APZCBasicTester(AsyncPanZoomController::GestureBehavior aGestureBehavior = AsyncPanZoomController::DEFAULT_GESTURES)
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

  void UseTouchListenerMetrics()
  {
    FrameMetrics frameMetrics(TestFrameMetrics());
    frameMetrics.mMayHaveTouchListeners = true;
    apzc->SetFrameMetrics(frameMetrics);
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
ApzcDown(AsyncPanZoomController* apzc, int aX, int aY, int& aTime)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0));
  return apzc->ReceiveInputEvent(mti);
}

static nsEventStatus
ApzcUp(AsyncPanZoomController* apzc, int aX, int aY, int& aTime)
{
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, aTime, TimeStamp(), 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0));
  return apzc->ReceiveInputEvent(mti);
}

static nsEventStatus
ApzcTap(AsyncPanZoomController* apzc, int aX, int aY, int& aTime,
        int aTapLength, MockContentControllerDelayed* mcc = nullptr)
{
  nsEventStatus status = ApzcDown(apzc, aX, aY, aTime);
  if (mcc != nullptr) {
    
    
    mcc->CheckHasDelayedTask();
    mcc->ClearDelayedTask();
    mcc->CheckHasDelayedTask();
    mcc->ClearDelayedTask();
  }
  EXPECT_EQ(nsEventStatus_eConsumeNoDefault, status);
  aTime += aTapLength;
  return ApzcUp(apzc, aX, aY, aTime);
}

static void
ApzcPan(AsyncPanZoomController* aApzc,
        TestAPZCTreeManager* aTreeManager,
        int& aTime,
        int aTouchStartY,
        int aTouchEndY,
        bool aKeepFingerDown = false,
        nsTArray<uint32_t>* aAllowedTouchBehaviors = nullptr,
        nsEventStatus (*aOutEventStatuses)[4] = nullptr)
{
  const int TIME_BETWEEN_TOUCH_EVENT = 100;
  const int OVERCOME_TOUCH_TOLERANCE = 100;

  
  
  
  aTreeManager->BuildOverscrollHandoffChain(aApzc);

  
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
    status = (nsEventStatus)-1;
  }
  if (aOutEventStatuses) {
    (*aOutEventStatuses)[3] = status;
  }

  aTime += TIME_BETWEEN_TOUCH_EVENT;

  
  
  aTreeManager->ClearOverscrollHandoffChain();
}





static void
ApzcPanAndCheckStatus(AsyncPanZoomController* aApzc,
                      TestAPZCTreeManager* aTreeManager,
                      int& aTime,
                      int aTouchStartY,
                      int aTouchEndY,
                      bool expectIgnoredPan,
                      bool hasTouchListeners,
                      nsTArray<uint32_t>* aAllowedTouchBehaviors)
{
  nsEventStatus statuses[4]; 
  ApzcPan(aApzc, aTreeManager, aTime, aTouchStartY, aTouchEndY, false, aAllowedTouchBehaviors, &statuses);

  nsEventStatus touchStartStatus;
  if (hasTouchListeners) {
    
    
    touchStartStatus = nsEventStatus_eIgnore;
  } else {
    
    touchStartStatus = nsEventStatus_eConsumeNoDefault;
  }
  EXPECT_EQ(touchStartStatus, statuses[0]);

  nsEventStatus touchMoveStatus;
  if (expectIgnoredPan) {
    
    
    touchMoveStatus = nsEventStatus_eIgnore;
  } else {
    
    touchMoveStatus = nsEventStatus_eConsumeNoDefault;
  }
  EXPECT_EQ(touchMoveStatus, statuses[1]);
  EXPECT_EQ(touchMoveStatus, statuses[2]);
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

  nsEventStatus expectedStatus = aShouldTriggerPinch
      ? nsEventStatus_eConsumeNoDefault
      : nsEventStatus_eIgnore;
  EXPECT_EQ(statuses[1], expectedStatus);
  EXPECT_EQ(statuses[2], expectedStatus);
}

class APZCPinchTester : public APZCBasicTester {
public:
  APZCPinchTester(AsyncPanZoomController::GestureBehavior aGestureBehavior = AsyncPanZoomController::DEFAULT_GESTURES)
    : APZCBasicTester(aGestureBehavior)
  {
  }

protected:
  void DoPinchTest(bool aShouldTriggerPinch,
                   nsTArray<uint32_t> *aAllowedTouchBehaviors = nullptr)
  {
    FrameMetrics fm;
    fm.mViewport = CSSRect(0, 0, 980, 480);
    fm.mCompositionBounds = ParentLayerRect(200, 200, 100, 200);
    fm.mScrollableRect = CSSRect(0, 0, 980, 1000);
    fm.SetScrollOffset(CSSPoint(300, 300));
    fm.SetZoom(CSSToScreenScale(2.0));
    apzc->SetFrameMetrics(fm);
    

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

    fm = apzc->GetFrameMetrics();

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

TEST_F(APZCBasicTester, Overzoom) {
  
  FrameMetrics fm;
  fm.mViewport = CSSRect(0, 0, 100, 100);
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
  gfx3DMatrix transforms[] = {
    gfx3DMatrix(),
    gfx3DMatrix(),
  };
  transforms[0].ScalePost(0.5f, 0.5f, 1.0f); 
  transforms[1].ScalePost(2.0f, 1.0f, 1.0f); 

  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root = CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, lm, layers);

  FrameMetrics metrics;
  metrics.mCompositionBounds = ParentLayerRect(0, 0, 24, 24);
  metrics.mDisplayPort = CSSRect(-1, -1, 6, 6);
  metrics.mViewport = CSSRect(0, 0, 4, 4);
  metrics.SetScrollOffset(CSSPoint(10, 10));
  metrics.mScrollableRect = CSSRect(0, 0, 50, 50);
  metrics.mCumulativeResolution = LayoutDeviceToLayerScale(2);
  metrics.mResolution = ParentLayerToLayerScale(2);
  metrics.SetZoom(CSSToScreenScale(6));
  metrics.mDevPixelsPerCSSPixel = CSSToLayoutDeviceScale(3);
  metrics.SetScrollId(FrameMetrics::START_SCROLL_ID);

  FrameMetrics childMetrics = metrics;
  childMetrics.SetScrollId(FrameMetrics::START_SCROLL_ID + 1);

  layers[0]->AsContainerLayer()->SetFrameMetrics(metrics);
  layers[1]->AsContainerLayer()->SetFrameMetrics(childMetrics);

  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  

  
  apzc->SetFrameMetrics(metrics);
  apzc->NotifyLayersUpdated(metrics, true);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(), ParentLayerToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(60, 60), pointOut);

  childApzc->SetFrameMetrics(childMetrics);
  childApzc->NotifyLayersUpdated(childMetrics, true);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(), ParentLayerToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(60, 60), pointOut);

  
  metrics.ScrollBy(CSSPoint(5, 0));
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  childMetrics.ScrollBy(CSSPoint(5, 0));
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  
  metrics.ZoomBy(1.5f);
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(3)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);

  childMetrics.ZoomBy(1.5f);
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(3)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);
}

class APZCPanningTester : public APZCBasicTester {
protected:
  void DoPanTest(bool aShouldTriggerScroll, uint32_t aBehavior)
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

    
    ApzcPanAndCheckStatus(apzc, tm, time, touchStart, touchEnd, !aShouldTriggerScroll, false, &allowedTouchBehaviors);
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    if (aShouldTriggerScroll) {
      EXPECT_EQ(ScreenPoint(0, -(touchEnd-touchStart)), pointOut);
      EXPECT_NE(ViewTransform(), viewTransformOut);
    } else {
      EXPECT_EQ(ScreenPoint(), pointOut);
      EXPECT_EQ(ViewTransform(), viewTransformOut);
    }

    
    ApzcPanAndCheckStatus(apzc, tm, time, touchEnd, touchStart, !aShouldTriggerScroll, false, &allowedTouchBehaviors);
    apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

    EXPECT_EQ(ScreenPoint(), pointOut);
    EXPECT_EQ(ViewTransform(), viewTransformOut);
  }
};

TEST_F(APZCPanningTester, Pan) {
  DoPanTest(true, mozilla::layers::AllowedTouchBehavior::NONE);
}






TEST_F(APZCPanningTester, PanWithTouchActionAuto) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(true, mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN
                  | mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
}

TEST_F(APZCPanningTester, PanWithTouchActionNone) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(false, 0);
}

TEST_F(APZCPanningTester, PanWithTouchActionPanX) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(false, mozilla::layers::AllowedTouchBehavior::HORIZONTAL_PAN);
}

TEST_F(APZCPanningTester, PanWithTouchActionPanY) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  DoPanTest(true, mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
}

TEST_F(APZCBasicTester, PanWithPreventDefault) {
  SCOPED_GFX_PREF(TouchActionEnabled, bool, true);
  UseTouchListenerMetrics();

  int time = 0;
  int touchStart = 50;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  nsTArray<uint32_t> allowedTouchBehaviors;
  allowedTouchBehaviors.AppendElement(mozilla::layers::AllowedTouchBehavior::VERTICAL_PAN);
  ApzcPanAndCheckStatus(apzc, tm, time, touchStart, touchEnd, true, true, &allowedTouchBehaviors);

  
  
  apzc->ContentReceivedTouch(true);

  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ScreenPoint(), pointOut);
  EXPECT_EQ(ViewTransform(), viewTransformOut);
}

TEST_F(APZCBasicTester, Fling) {
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  int time = 0;
  int touchStart = 50;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  ApzcPan(apzc, tm, time, touchStart, touchEnd);
  ScreenPoint lastPoint;
  for (int i = 1; i < 50; i+=1) {
    apzc->SampleContentTransformForFrame(testStartTime+TimeDuration::FromMilliseconds(i), &viewTransformOut, pointOut);
    EXPECT_GT(pointOut.y, lastPoint.y);
    lastPoint = pointOut;
  }
}

class APZCFlingStopTester : public APZCGestureDetectorTester {
protected:
  
  
  
  
  
  
  void DoFlingStopTest(bool aSlow) {
    int time = 0;
    int touchStart = 50;
    int touchEnd = 10;

    
    ApzcPan(apzc, tm, time, touchStart, touchEnd);
    
    EXPECT_EQ(2, mcc->RunThroughDelayedTasks());

    
    
    
    int timeDelta = aSlow ? 2000 : 10;
    int tapCallsExpected = aSlow ? 1 : 0;
    int delayedTasksExpected = aSlow ? 3 : 2;

    
    ScreenPoint pointOut;
    ViewTransform viewTransformOut;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(timeDelta), &viewTransformOut, pointOut);

    
    
    EXPECT_CALL(*mcc, HandleSingleTap(_, 0, apzc->GetGuid())).Times(tapCallsExpected);
    ApzcTap(apzc, 10, 10, time, 0, nullptr);
    EXPECT_EQ(delayedTasksExpected, mcc->RunThroughDelayedTasks());

    
    ScreenPoint finalPointOut;
    apzc->SampleContentTransformForFrame(testStartTime + TimeDuration::FromMilliseconds(timeDelta + 1000), &viewTransformOut, finalPointOut);
    EXPECT_EQ(pointOut.x, finalPointOut.x);
    EXPECT_EQ(pointOut.y, finalPointOut.y);

    apzc->AssertStateIsReset();
  }
};

TEST_F(APZCFlingStopTester, FlingStop) {
  DoFlingStopTest(false);
}

TEST_F(APZCFlingStopTester, FlingStopTap) {
  DoFlingStopTest(true);
}

TEST_F(APZCBasicTester, OverScrollPanning) {
  SCOPED_GFX_PREF(APZOverscrollEnabled, bool, true);

  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  ApzcPan(apzc, tm, time, touchStart, touchEnd);
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
  ApzcPan(apzc, tm, time, touchStart, touchEnd);
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
  ApzcPan(apzc, tm, time, touchStart, touchEnd,
          true);                   
  EXPECT_TRUE(apzc->IsOverscrolled());

  
  
  
  apzc->CancelAnimation();
  EXPECT_FALSE(apzc->IsOverscrolled());
  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, ShortPress) {
  MakeApzcUnzoomable();

  int time = 0;
  nsEventStatus status = ApzcTap(apzc, 10, 10, time, 100, mcc.get());
  EXPECT_EQ(nsEventStatus_eIgnore, status);

  
  
  mcc->CheckHasDelayedTask();

  EXPECT_CALL(*mcc, HandleSingleTap(CSSPoint(10, 10), 0, apzc->GetGuid())).Times(1);
  mcc->RunDelayedTask();

  apzc->AssertStateIsReset();
}

TEST_F(APZCGestureDetectorTester, MediumPress) {
  MakeApzcUnzoomable();

  int time = 0;
  nsEventStatus status = ApzcTap(apzc, 10, 10, time, 400, mcc.get());
  EXPECT_EQ(nsEventStatus_eIgnore, status);

  
  
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
    EXPECT_EQ(nsEventStatus_eConsumeNoDefault, status);

    
    nsTArray<uint32_t> allowedTouchBehaviors;
    allowedTouchBehaviors.AppendElement(aBehavior);
    apzc->SetAllowedTouchBehavior(allowedTouchBehaviors);

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
    
    
    
    
    mcc->CheckHasDelayedTask();
    mcc->ClearDelayedTask();
    apzc->ContentReceivedTouch(true);

    time += 1000;

    status = ApzcUp(apzc, 10, 10, time);
    EXPECT_EQ(nsEventStatus_eIgnore, status);

    
    
    
    check.Call("preHandleLongTapUp");
    apzc->ContentReceivedTouch(false);
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
    EXPECT_EQ(nsEventStatus_eConsumeNoDefault, status);

    
    nsTArray<uint32_t> allowedTouchBehaviors;
    allowedTouchBehaviors.AppendElement(aBehavior);
    apzc->SetAllowedTouchBehavior(allowedTouchBehaviors);

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
    
    
    
    mcc->ClearDelayedTask();
    apzc->ContentReceivedTouch(true);

    time += 1000;

    MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, time, TimeStamp(), 0);
    mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(touchX, touchEndY), ScreenSize(0, 0), 0, 0));
    status = apzc->ReceiveInputEvent(mti);
    EXPECT_EQ(nsEventStatus_eIgnore, status);

    EXPECT_CALL(*mcc, HandleLongTapUp(CSSPoint(touchX, touchEndY), 0, apzc->GetGuid())).Times(0);
    status = ApzcUp(apzc, touchX, touchEndY, time);
    EXPECT_EQ(nsEventStatus_eIgnore, status);

    
    
    
    apzc->ContentReceivedTouch(false);

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


static already_AddRefed<mozilla::layers::Layer>
CreateTestLayerTree1(nsRefPtr<LayerManager>& aLayerManager, nsTArray<nsRefPtr<Layer> >& aLayers) {
  const char* layerTreeSyntax = "c(ttcc)";
  
  nsIntRegion layerVisibleRegion[] = {
    nsIntRegion(nsIntRect(0,0,100,100)),
    nsIntRegion(nsIntRect(0,0,100,100)),
    nsIntRegion(nsIntRect(10,10,20,20)),
    nsIntRegion(nsIntRect(10,10,20,20)),
    nsIntRegion(nsIntRect(5,5,20,20)),
  };
  gfx3DMatrix transforms[] = {
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
  };
  return CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, aLayerManager, aLayers);
}


static already_AddRefed<mozilla::layers::Layer>
CreateTestLayerTree2(nsRefPtr<LayerManager>& aLayerManager, nsTArray<nsRefPtr<Layer> >& aLayers) {
  const char* layerTreeSyntax = "c(cc(c))";
  
  nsIntRegion layerVisibleRegion[] = {
    nsIntRegion(nsIntRect(0,0,100,100)),
    nsIntRegion(nsIntRect(10,10,40,40)),
    nsIntRegion(nsIntRect(10,60,40,40)),
    nsIntRegion(nsIntRect(10,60,40,40)),
  };
  gfx3DMatrix transforms[] = {
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
  };
  return CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, aLayerManager, aLayers);
}

static void
SetScrollableFrameMetrics(Layer* aLayer, FrameMetrics::ViewID aScrollId,
                          
                          
                          CSSRect aScrollableRect = CSSRect(-1, -1, -1, -1))
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  FrameMetrics metrics;
  metrics.SetScrollId(aScrollId);
  nsIntRect layerBound = aLayer->GetVisibleRegion().GetBounds();
  metrics.mCompositionBounds = ParentLayerRect(layerBound.x, layerBound.y,
                                               layerBound.width, layerBound.height);
  metrics.mScrollableRect = aScrollableRect;
  metrics.SetScrollOffset(CSSPoint(0, 0));
  container->SetFrameMetrics(metrics);
}

static already_AddRefed<AsyncPanZoomController>
GetTargetAPZC(APZCTreeManager* manager, const ScreenPoint& aPoint,
              gfx3DMatrix& aTransformToApzcOut, gfx3DMatrix& aTransformToGeckoOut)
{
  nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(aPoint, nullptr);
  if (hit) {
    manager->GetInputTransforms(hit.get(), aTransformToApzcOut, aTransformToGeckoOut);
  }
  return hit.forget();
}


TEST_F(APZCTreeManagerTester, HitTesting1) {
  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root = CreateTestLayerTree1(lm, layers);

  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  ScopedLayerTreeRegistration controller(0, root, mcc);

  nsRefPtr<APZCTreeManager> manager = new TestAPZCTreeManager();
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;

  
  nsRefPtr<AsyncPanZoomController> hit = GetTargetAPZC(manager, ScreenPoint(20, 20), transformToApzc, transformToGecko);
  AsyncPanZoomController* nullAPZC = nullptr;
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(gfx3DMatrix(), transformToApzc);
  EXPECT_EQ(gfx3DMatrix(), transformToGecko);

  uint32_t paintSequenceNumber = 0;

  
  SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, paintSequenceNumber++);
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(root->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  
  EXPECT_EQ(gfxPoint(15, 15), transformToApzc.Transform(gfxPoint(15, 15)));
  EXPECT_EQ(gfxPoint(15, 15), transformToGecko.Transform(gfxPoint(15, 15)));

  
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, paintSequenceNumber++);
  EXPECT_NE(root->AsContainerLayer()->GetAsyncPanZoomController(), layers[3]->AsContainerLayer()->GetAsyncPanZoomController());
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  
  EXPECT_EQ(gfxPoint(15, 15), transformToApzc.Transform(gfxPoint(15, 15)));
  EXPECT_EQ(gfxPoint(15, 15), transformToGecko.Transform(gfxPoint(15, 15)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 2);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, paintSequenceNumber++);
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(layers[4]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  
  EXPECT_EQ(gfxPoint(15, 15), transformToApzc.Transform(gfxPoint(15, 15)));
  EXPECT_EQ(gfxPoint(15, 15), transformToGecko.Transform(gfxPoint(15, 15)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(90, 90), transformToApzc, transformToGecko);
  EXPECT_EQ(root->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  
  EXPECT_EQ(gfxPoint(90, 90), transformToApzc.Transform(gfxPoint(90, 90)));
  EXPECT_EQ(gfxPoint(90, 90), transformToGecko.Transform(gfxPoint(90, 90)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(1000, 10), transformToApzc, transformToGecko);
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(gfx3DMatrix(), transformToApzc);
  EXPECT_EQ(gfx3DMatrix(), transformToGecko);
  hit = GetTargetAPZC(manager, ScreenPoint(-1000, 10), transformToApzc, transformToGecko);
  EXPECT_EQ(nullAPZC, hit.get());
  EXPECT_EQ(gfx3DMatrix(), transformToApzc);
  EXPECT_EQ(gfx3DMatrix(), transformToGecko);

  manager->ClearTree();
}


TEST_F(APZCTreeManagerTester, HitTesting2) {
  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root = CreateTestLayerTree2(lm, layers);

  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  ScopedLayerTreeRegistration controller(0, root, mcc);

  nsRefPtr<TestAPZCTreeManager> manager = new TestAPZCTreeManager();
  nsRefPtr<AsyncPanZoomController> hit;
  gfx3DMatrix transformToApzc;
  gfx3DMatrix transformToGecko;

  
  Matrix4x4 transform;
  transform = transform * Matrix4x4().Scale(2, 1, 1);
  layers[2]->SetBaseTransform(transform);

  
  SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
  SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 80, 80));
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 0, 80, 80));

  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0, 0);

  
  
  
  
  

  AsyncPanZoomController* apzcroot = root->AsContainerLayer()->GetAsyncPanZoomController();
  AsyncPanZoomController* apzc1 = layers[1]->AsContainerLayer()->GetAsyncPanZoomController();
  AsyncPanZoomController* apzc3 = layers[3]->AsContainerLayer()->GetAsyncPanZoomController();

  
  hit = GetTargetAPZC(manager, ScreenPoint(75, 25), transformToApzc, transformToGecko);
  EXPECT_EQ(apzcroot, hit.get());
  EXPECT_EQ(gfxPoint(75, 25), transformToApzc.Transform(gfxPoint(75, 25)));
  EXPECT_EQ(gfxPoint(75, 25), transformToGecko.Transform(gfxPoint(75, 25)));

  
  
  
  
  
  
  
  hit = GetTargetAPZC(manager, ScreenPoint(15, 75), transformToApzc, transformToGecko);
  EXPECT_EQ(apzcroot, hit.get());
  EXPECT_EQ(gfxPoint(15, 75), transformToApzc.Transform(gfxPoint(15, 75)));
  EXPECT_EQ(gfxPoint(15, 75), transformToGecko.Transform(gfxPoint(15, 75)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(25, 25), transformToApzc, transformToGecko);
  EXPECT_EQ(apzc1, hit.get());
  EXPECT_EQ(gfxPoint(25, 25), transformToApzc.Transform(gfxPoint(25, 25)));
  EXPECT_EQ(gfxPoint(25, 25), transformToGecko.Transform(gfxPoint(25, 25)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(25, 75), transformToApzc, transformToGecko);
  EXPECT_EQ(apzc3, hit.get());
  
  EXPECT_EQ(gfxPoint(12.5, 75), transformToApzc.Transform(gfxPoint(25, 75)));
  
  EXPECT_EQ(gfxPoint(25, 75), transformToGecko.Transform(gfxPoint(12.5, 75)));

  
  
  hit = GetTargetAPZC(manager, ScreenPoint(75, 75), transformToApzc, transformToGecko);
  EXPECT_EQ(apzc3, hit.get());
  
  EXPECT_EQ(gfxPoint(37.5, 75), transformToApzc.Transform(gfxPoint(75, 75)));
  
  EXPECT_EQ(gfxPoint(75, 75), transformToGecko.Transform(gfxPoint(37.5, 75)));

  
  
  
  int time = 0;
  
  EXPECT_CALL(*mcc, PostDelayedTask(_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  
  
  
  ApzcPan(apzcroot, manager, time, 100, 50);

  
  hit = GetTargetAPZC(manager, ScreenPoint(75, 75), transformToApzc, transformToGecko);
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(gfxPoint(75, 75), transformToApzc.Transform(gfxPoint(75, 75)));
  
  
  
  EXPECT_EQ(gfxPoint(75, 75), transformToGecko.Transform(gfxPoint(75, 75)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(25, 25), transformToApzc, transformToGecko);
  EXPECT_EQ(apzc3, hit.get());
  
  
  EXPECT_EQ(gfxPoint(12.5, 75), transformToApzc.Transform(gfxPoint(25, 25)));
  
  
  EXPECT_EQ(gfxPoint(25, 25), transformToGecko.Transform(gfxPoint(12.5, 75)));

  
  
  
  
  ApzcPan(apzcroot, manager, time, 100, 50);

  
  hit = GetTargetAPZC(manager, ScreenPoint(75, 75), transformToApzc, transformToGecko);
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(gfxPoint(75, 75), transformToApzc.Transform(gfxPoint(75, 75)));
  
  
  EXPECT_EQ(gfxPoint(75, 125), transformToGecko.Transform(gfxPoint(75, 75)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(25, 25), transformToApzc, transformToGecko);
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(gfxPoint(25, 25), transformToApzc.Transform(gfxPoint(25, 25)));
  
  
  EXPECT_EQ(gfxPoint(25, 75), transformToGecko.Transform(gfxPoint(25, 25)));

  manager->ClearTree();
}
