




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/Attributes.h"
#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "base/task.h"
#include "Layers.h"
#include "TestLayers.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using ::testing::_;
using ::testing::NiceMock; 
using ::testing::AtLeast;

class Task;

class MockContentController : public GeckoContentController {
public:
  MOCK_METHOD1(RequestContentRepaint, void(const FrameMetrics&));
  MOCK_METHOD2(HandleDoubleTap, void(const CSSIntPoint&, int32_t));
  MOCK_METHOD2(HandleSingleTap, void(const CSSIntPoint&, int32_t));
  MOCK_METHOD2(HandleLongTap, void(const CSSIntPoint&, int32_t));
  MOCK_METHOD2(HandleLongTapUp, void(const CSSIntPoint&, int32_t));
  MOCK_METHOD3(SendAsyncScrollDOMEvent, void(bool aIsRoot, const CSSRect &aContentRect, const CSSSize &aScrollableSize));
  MOCK_METHOD2(PostDelayedTask, void(Task* aTask, int aDelayMs));
};

class MockContentControllerDelayed : public MockContentController {
public:

  void PostDelayedTask(Task* aTask, int aDelayMs) {
    mCurrentTask = aTask;
  }

  Task* GetDelayedTask() {
    return mCurrentTask;
  }

private:
  Task *mCurrentTask;
};


class TestAPZCContainerLayer : public ContainerLayer {
  public:
    TestAPZCContainerLayer()
      : ContainerLayer(nullptr, nullptr)
    {}
  void RemoveChild(Layer* aChild) {}
  void InsertAfter(Layer* aChild, Layer* aAfter) {}
  void ComputeEffectiveTransforms(const gfx3DMatrix& aTransformToSurface) {}
  void RepositionChild(Layer* aChild, Layer* aAfter) {}
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
};

class TestAPZCTreeManager : public APZCTreeManager {
protected:
  void AssertOnCompositorThread() MOZ_OVERRIDE {  }

public:
  
  void BuildOverscrollHandoffChain(AsyncPanZoomController* aApzc) {
    APZCTreeManager::BuildOverscrollHandoffChain(aApzc);
  }
};

static
FrameMetrics TestFrameMetrics() {
  FrameMetrics fm;

  fm.mDisplayPort = CSSRect(0, 0, 10, 10);
  fm.mCompositionBounds = ScreenIntRect(0, 0, 10, 10);
  fm.mCriticalDisplayPort = CSSRect(0, 0, 10, 10);
  fm.mScrollableRect = CSSRect(0, 0, 100, 100);
  fm.mViewport = CSSRect(0, 0, 10, 10);

  return fm;
}

static
void ApzcPan(AsyncPanZoomController* apzc, TestAPZCTreeManager* aTreeManager, int& aTime, int aTouchStartY, int aTouchEndY) {

  const int TIME_BETWEEN_TOUCH_EVENT = 100;
  const int OVERCOME_TOUCH_TOLERANCE = 100;
  MultiTouchInput mti;
  nsEventStatus status;

  
  
  
  aTreeManager->BuildOverscrollHandoffChain(apzc);

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, aTime, 0);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchStartY+OVERCOME_TOUCH_TOLERANCE), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
  EXPECT_EQ(status, nsEventStatus_eConsumeNoDefault);
  

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime, 0);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchStartY), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
  EXPECT_EQ(status, nsEventStatus_eConsumeNoDefault);
  

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime, 0);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchEndY), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
  EXPECT_EQ(status, nsEventStatus_eConsumeNoDefault);

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, aTime, 0);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchEndY), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
}

static void
ApzcPinch(AsyncPanZoomController* aApzc, int aFocusX, int aFocusY, float aScale) {
  aApzc->HandleInputEvent(PinchGestureInput(PinchGestureInput::PINCHGESTURE_START,
                                            0,
                                            ScreenPoint(aFocusX, aFocusY),
                                            10.0,
                                            10.0,
                                            0));
  aApzc->HandleInputEvent(PinchGestureInput(PinchGestureInput::PINCHGESTURE_SCALE,
                                            0,
                                            ScreenPoint(aFocusX, aFocusY),
                                            10.0 * aScale,
                                            10.0,
                                            0));
  aApzc->HandleInputEvent(PinchGestureInput(PinchGestureInput::PINCHGESTURE_END,
                                            0,
                                            ScreenPoint(aFocusX, aFocusY),
                                            
                                            
                                            -1.0,
                                            -1.0,
                                            0));
}

static nsEventStatus
ApzcDown(AsyncPanZoomController* apzc, int aX, int aY, int& aTime) {
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, aTime, 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0));
  return apzc->ReceiveInputEvent(mti);
}

static nsEventStatus
ApzcUp(AsyncPanZoomController* apzc, int aX, int aY, int& aTime) {
  MultiTouchInput mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, aTime, 0);
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(aX, aY), ScreenSize(0, 0), 0, 0));
  return apzc->ReceiveInputEvent(mti);
}

static nsEventStatus
ApzcTap(AsyncPanZoomController* apzc, int aX, int aY, int& aTime, int aTapLength) {
  nsEventStatus status = ApzcDown(apzc, aX, aY, aTime);
  EXPECT_EQ(nsEventStatus_eConsumeNoDefault, status);
  aTime += aTapLength;
  return ApzcUp(apzc, aX, aY, aTime);
}

TEST(AsyncPanZoomController, Constructor) {
  
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);
  apzc->SetFrameMetrics(TestFrameMetrics());
}

TEST(AsyncPanZoomController, Pinch) {
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);

  FrameMetrics fm;
  fm.mViewport = CSSRect(0, 0, 980, 480);
  fm.mCompositionBounds = ScreenIntRect(200, 200, 100, 200);
  fm.mScrollableRect = CSSRect(0, 0, 980, 1000);
  fm.mScrollOffset = CSSPoint(300, 300);
  fm.mZoom = CSSToScreenScale(2.0);
  apzc->SetFrameMetrics(fm);
  

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  ApzcPinch(apzc, 250, 300, 1.25);

  
  fm = apzc->GetFrameMetrics();
  EXPECT_EQ(fm.mZoom.scale, 2.5f);
  EXPECT_EQ(fm.mScrollOffset.x, 305);
  EXPECT_EQ(fm.mScrollOffset.y, 310);

  
  
  fm.mZoom = CSSToScreenScale(2.0);
  fm.mScrollOffset = CSSPoint(930, 5);
  apzc->SetFrameMetrics(fm);
  

  ApzcPinch(apzc, 250, 300, 0.5);

  
  fm = apzc->GetFrameMetrics();
  EXPECT_EQ(fm.mZoom.scale, 1.0f);
  EXPECT_EQ(fm.mScrollOffset.x, 880);
  EXPECT_EQ(fm.mScrollOffset.y, 0);
}

TEST(AsyncPanZoomController, Overzoom) {
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);

  FrameMetrics fm;
  fm.mViewport = CSSRect(0, 0, 100, 100);
  fm.mCompositionBounds = ScreenIntRect(0, 0, 100, 100);
  fm.mScrollableRect = CSSRect(0, 0, 125, 150);
  fm.mScrollOffset = CSSPoint(10, 0);
  fm.mZoom = CSSToScreenScale(1.0);
  apzc->SetFrameMetrics(fm);
  

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  ApzcPinch(apzc, 50, 50, 0.5);

  fm = apzc->GetFrameMetrics();
  EXPECT_EQ(fm.mZoom.scale, 0.8f);
  
  
  EXPECT_LT(abs(fm.mScrollOffset.x), 1e-5);
  EXPECT_LT(abs(fm.mScrollOffset.y), 1e-5);
}

TEST(AsyncPanZoomController, SimpleTransform) {
  TimeStamp testStartTime = TimeStamp::Now();
  
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);
  apzc->SetFrameMetrics(TestFrameMetrics());

  ScreenPoint pointOut;
  ViewTransform viewTransformOut;
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);

  EXPECT_EQ(pointOut, ScreenPoint());
  EXPECT_EQ(viewTransformOut, ViewTransform());
}


TEST(AsyncPanZoomController, ComplexTransform) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);
  nsRefPtr<TestAsyncPanZoomController> childApzc = new TestAsyncPanZoomController(0, mcc);

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
  metrics.mCompositionBounds = ScreenIntRect(0, 0, 24, 24);
  metrics.mDisplayPort = CSSRect(-1, -1, 6, 6);
  metrics.mViewport = CSSRect(0, 0, 4, 4);
  metrics.mScrollOffset = CSSPoint(10, 10);
  metrics.mScrollableRect = CSSRect(0, 0, 50, 50);
  metrics.mCumulativeResolution = LayoutDeviceToLayerScale(2);
  metrics.mResolution = ParentLayerToLayerScale(2);
  metrics.mZoom = CSSToScreenScale(6);
  metrics.mDevPixelsPerCSSPixel = CSSToLayoutDeviceScale(3);
  metrics.mScrollId = FrameMetrics::START_SCROLL_ID;

  FrameMetrics childMetrics = metrics;
  childMetrics.mScrollId = FrameMetrics::START_SCROLL_ID + 1;

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

  
  metrics.mScrollOffset += CSSPoint(5, 0);
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  childMetrics.mScrollOffset += CSSPoint(5, 0);
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  
  metrics.mZoom.scale *= 1.5f;
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(3)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);

  childMetrics.mZoom.scale *= 1.5f;
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), ParentLayerToScreenScale(3)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);
}

TEST(AsyncPanZoomController, Pan) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAPZCTreeManager> tm = new TestAPZCTreeManager();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc, tm);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  int time = 0;
  int touchStart = 50;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  ApzcPan(apzc, tm, time, touchStart, touchEnd);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(pointOut, ScreenPoint(0, -(touchEnd-touchStart)));
  EXPECT_NE(viewTransformOut, ViewTransform());

  
  ApzcPan(apzc, tm, time, touchEnd, touchStart);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(pointOut, ScreenPoint());
  EXPECT_EQ(viewTransformOut, ViewTransform());
}

TEST(AsyncPanZoomController, Fling) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAPZCTreeManager> tm = new TestAPZCTreeManager();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc, tm);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);

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

TEST(AsyncPanZoomController, OverScrollPanning) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAPZCTreeManager> tm = new TestAPZCTreeManager();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc, tm);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  ApzcPan(apzc, tm, time, touchStart, touchEnd);
  apzc->SampleContentTransformForFrame(testStartTime+TimeDuration::FromMilliseconds(1000), &viewTransformOut, pointOut);
  EXPECT_EQ(pointOut, ScreenPoint(0, 90));
}

TEST(AsyncPanZoomController, ShortPress) {
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAPZCTreeManager> tm = new TestAPZCTreeManager();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(
    0, mcc, tm, AsyncPanZoomController::USE_GESTURE_DETECTOR);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);
  apzc->UpdateZoomConstraints(false, CSSToScreenScale(1.0), CSSToScreenScale(1.0));

  EXPECT_CALL(*mcc, HandleSingleTap(CSSIntPoint(10, 10), 0)).Times(1);

  int time = 0;
  nsEventStatus status = ApzcTap(apzc, 10, 10, time, 100);
  EXPECT_EQ(nsEventStatus_eIgnore, status);

  apzc->Destroy();
}

TEST(AsyncPanZoomController, MediumPress) {
  nsRefPtr<MockContentController> mcc = new NiceMock<MockContentController>();
  nsRefPtr<TestAPZCTreeManager> tm = new TestAPZCTreeManager();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(
    0, mcc, tm, AsyncPanZoomController::USE_GESTURE_DETECTOR);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);
  apzc->UpdateZoomConstraints(false, CSSToScreenScale(1.0), CSSToScreenScale(1.0));

  EXPECT_CALL(*mcc, HandleSingleTap(CSSIntPoint(10, 10), 0)).Times(1);

  int time = 0;
  nsEventStatus status = ApzcTap(apzc, 10, 10, time, 400);
  EXPECT_EQ(nsEventStatus_eIgnore, status);

  apzc->Destroy();
}

TEST(AsyncPanZoomController, LongPress) {
  nsRefPtr<MockContentControllerDelayed> mcc = new MockContentControllerDelayed();
  nsRefPtr<TestAPZCTreeManager> tm = new TestAPZCTreeManager();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(
    0, mcc, tm, AsyncPanZoomController::USE_GESTURE_DETECTOR);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);
  apzc->UpdateZoomConstraints(false, CSSToScreenScale(1.0), CSSToScreenScale(1.0));

  int time = 0;

  nsEventStatus status = ApzcDown(apzc, 10, 10, time);
  EXPECT_EQ(nsEventStatus_eConsumeNoDefault, status);

  Task* t = mcc->GetDelayedTask();

  EXPECT_TRUE(nullptr != t);
  EXPECT_CALL(*mcc, HandleLongTap(CSSIntPoint(10, 10), 0)).Times(1);
  EXPECT_CALL(*mcc, HandleLongTapUp(CSSIntPoint(10, 10), 0)).Times(1);
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));

  
  t->Run();

  time += 1000;

  status = ApzcUp(apzc, 10, 10, time);
  EXPECT_EQ(nsEventStatus_eIgnore, status);

  apzc->Destroy();
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
  metrics.mScrollId = aScrollId;
  nsIntRect layerBound = aLayer->GetVisibleRegion().GetBounds();
  metrics.mCompositionBounds = ScreenIntRect(layerBound.x, layerBound.y,
                                             layerBound.width, layerBound.height);
  metrics.mScrollableRect = aScrollableRect;
  metrics.mScrollOffset = CSSPoint(0, 0);
  container->SetFrameMetrics(metrics);
}

static already_AddRefed<AsyncPanZoomController>
GetTargetAPZC(APZCTreeManager* manager, const ScreenPoint& aPoint,
              gfx3DMatrix& aTransformToApzcOut, gfx3DMatrix& aTransformToGeckoOut)
{
  nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(aPoint);
  if (hit) {
    manager->GetInputTransforms(hit.get(), aTransformToApzcOut, aTransformToGeckoOut);
  }
  return hit.forget();
}


TEST(APZCTreeManager, HitTesting1) {
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

  
  SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0);
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(root->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  
  EXPECT_EQ(gfxPoint(15, 15), transformToApzc.Transform(gfxPoint(15, 15)));
  EXPECT_EQ(gfxPoint(15, 15), transformToGecko.Transform(gfxPoint(15, 15)));

  
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 1);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0);
  EXPECT_NE(root->AsContainerLayer()->GetAsyncPanZoomController(), layers[3]->AsContainerLayer()->GetAsyncPanZoomController());
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  
  EXPECT_EQ(gfxPoint(15, 15), transformToApzc.Transform(gfxPoint(15, 15)));
  EXPECT_EQ(gfxPoint(15, 15), transformToGecko.Transform(gfxPoint(15, 15)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(15, 15), transformToApzc, transformToGecko);
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 2);
  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0);
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


TEST(APZCTreeManager, HitTesting2) {
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

  
  gfx3DMatrix transform;
  transform.ScalePost(2, 1, 1);
  layers[2]->SetBaseTransform(transform);

  
  SetScrollableFrameMetrics(root, FrameMetrics::START_SCROLL_ID, CSSRect(0, 0, 200, 200));
  SetScrollableFrameMetrics(layers[1], FrameMetrics::START_SCROLL_ID + 1, CSSRect(0, 0, 80, 80));
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID + 2, CSSRect(0, 0, 80, 80));

  manager->UpdatePanZoomControllerTree(nullptr, root, false, 0);

  
  
  
  
  

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
  
  EXPECT_CALL(*mcc, PostDelayedTask(_,_)).Times(1);
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_,_)).Times(AtLeast(1));
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);
  ApzcPan(apzcroot, manager, time, 100, 50);

  
  hit = GetTargetAPZC(manager, ScreenPoint(75, 75), transformToApzc, transformToGecko);
  EXPECT_EQ(apzcroot, hit.get());
  
  EXPECT_EQ(gfxPoint(75, 75), transformToApzc.Transform(gfxPoint(75, 75)));
  
  EXPECT_EQ(gfxPoint(75, 125), transformToGecko.Transform(gfxPoint(75, 75)));

  
  hit = GetTargetAPZC(manager, ScreenPoint(25, 25), transformToApzc, transformToGecko);
  EXPECT_EQ(apzc3, hit.get());
  
  
  EXPECT_EQ(gfxPoint(12.5, 75), transformToApzc.Transform(gfxPoint(25, 25)));
  
  
  EXPECT_EQ(gfxPoint(25, 75), transformToGecko.Transform(gfxPoint(12.5, 75)));

  manager->ClearTree();
}
