




#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "mozilla/layers/AsyncCompositionManager.h" 
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/LayerManagerComposite.h"
#include "mozilla/layers/GeckoContentController.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/APZCTreeManager.h"
#include "Layers.h"
#include "TestLayers.h"

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;
using ::testing::_;

class MockContentController : public GeckoContentController {
public:
  MOCK_METHOD1(RequestContentRepaint, void(const FrameMetrics&));
  MOCK_METHOD1(HandleDoubleTap, void(const CSSIntPoint&));
  MOCK_METHOD1(HandleSingleTap, void(const CSSIntPoint&));
  MOCK_METHOD1(HandleLongTap, void(const CSSIntPoint&));
  MOCK_METHOD2(SendAsyncScrollDOMEvent, void(const CSSRect &aContentRect, const CSSSize &aScrollableSize));
  MOCK_METHOD2(PostDelayedTask, void(Task* aTask, int aDelayMs));
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
  TestAsyncPanZoomController(uint64_t aLayersId, MockContentController* mcc)
    : AsyncPanZoomController(aLayersId, mcc)
  {}

  void SetFrameMetrics(const FrameMetrics& metrics) {
    MonitorAutoLock lock(mMonitor);
    mFrameMetrics = metrics;
  }
};

class TestAPZCTreeManager : public APZCTreeManager {
protected:
  void AssertOnCompositorThread() MOZ_OVERRIDE {  }
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
void ApzcPan(AsyncPanZoomController* apzc, int& aTime, int aTouchStartY, int aTouchEndY) {

  const int TIME_BETWEEN_TOUCH_EVENT = 100;
  const int OVERCOME_TOUCH_TOLERANCE = 100;
  MultiTouchInput mti;
  nsEventStatus status;

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_START, aTime);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchStartY+OVERCOME_TOUCH_TOLERANCE), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
  EXPECT_EQ(status, nsEventStatus_eConsumeNoDefault);
  

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchStartY), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
  EXPECT_EQ(status, nsEventStatus_eConsumeNoDefault);
  

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_MOVE, aTime);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchEndY), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
  EXPECT_EQ(status, nsEventStatus_eConsumeNoDefault);

  mti = MultiTouchInput(MultiTouchInput::MULTITOUCH_END, aTime);
  aTime += TIME_BETWEEN_TOUCH_EVENT;
  mti.mTouches.AppendElement(SingleTouchData(0, ScreenIntPoint(10, aTouchEndY), ScreenSize(0, 0), 0, 0));
  status = apzc->HandleInputEvent(mti);
}

TEST(AsyncPanZoomController, Constructor) {
  
  nsRefPtr<MockContentController> mcc = new MockContentController();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);
  apzc->SetFrameMetrics(TestFrameMetrics());
}

TEST(AsyncPanZoomController, SimpleTransform) {
  TimeStamp testStartTime = TimeStamp::Now();
  
  nsRefPtr<MockContentController> mcc = new MockContentController();
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsRefPtr<MockContentController> mcc = new MockContentController();
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
  metrics.mResolution = LayoutDeviceToLayerScale(2);
  metrics.mZoom = ScreenToScreenScale(1);
  metrics.mDevPixelsPerCSSPixel = CSSToLayoutDeviceScale(3);
  metrics.mScrollId = FrameMetrics::ROOT_SCROLL_ID;

  FrameMetrics childMetrics = metrics;
  childMetrics.mScrollId = FrameMetrics::START_SCROLL_ID;

  layers[0]->AsContainerLayer()->SetFrameMetrics(metrics);
  layers[1]->AsContainerLayer()->SetFrameMetrics(childMetrics);

  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  

  
  apzc->SetFrameMetrics(metrics);
  apzc->NotifyLayersUpdated(metrics, true);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(), LayoutDeviceToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(60, 60), pointOut);

  childApzc->SetFrameMetrics(childMetrics);
  childApzc->NotifyLayersUpdated(childMetrics, true);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(), LayoutDeviceToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(60, 60), pointOut);

  
  metrics.mScrollOffset += CSSPoint(5, 0);
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), LayoutDeviceToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  childMetrics.mScrollOffset += CSSPoint(5, 0);
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), LayoutDeviceToScreenScale(2)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(90, 60), pointOut);

  
  metrics.mZoom.scale *= 1.5f;
  apzc->SetFrameMetrics(metrics);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), LayoutDeviceToScreenScale(3)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);

  childMetrics.mZoom.scale *= 1.5f;
  childApzc->SetFrameMetrics(childMetrics);
  childApzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(ViewTransform(LayerPoint(-30, 0), LayoutDeviceToScreenScale(3)), viewTransformOut);
  EXPECT_EQ(ScreenPoint(135, 90), pointOut);
}

TEST(AsyncPanZoomController, Pan) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  nsRefPtr<MockContentController> mcc = new MockContentController();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_)).Times(4);
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  int time = 0;
  int touchStart = 50;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  ApzcPan(apzc, time, touchStart, touchEnd);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(pointOut, ScreenPoint(0, -(touchEnd-touchStart)));
  EXPECT_NE(viewTransformOut, ViewTransform());

  
  ApzcPan(apzc, time, touchEnd, touchStart);
  apzc->SampleContentTransformForFrame(testStartTime, &viewTransformOut, pointOut);
  EXPECT_EQ(pointOut, ScreenPoint());
  EXPECT_EQ(viewTransformOut, ViewTransform());
}

TEST(AsyncPanZoomController, Fling) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  nsRefPtr<MockContentController> mcc = new MockContentController();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_)).Times(2);
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

TEST(AsyncPanZoomController, OverScrollPanning) {
  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);

  nsRefPtr<MockContentController> mcc = new MockContentController();
  nsRefPtr<TestAsyncPanZoomController> apzc = new TestAsyncPanZoomController(0, mcc);

  apzc->SetFrameMetrics(TestFrameMetrics());
  apzc->NotifyLayersUpdated(TestFrameMetrics(), true);

  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_)).Times(3);
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);

  
  int time = 0;
  int touchStart = 500;
  int touchEnd = 10;
  ScreenPoint pointOut;
  ViewTransform viewTransformOut;

  
  ApzcPan(apzc, time, touchStart, touchEnd);
  apzc->SampleContentTransformForFrame(testStartTime+TimeDuration::FromMilliseconds(1000), &viewTransformOut, pointOut);
  EXPECT_EQ(pointOut, ScreenPoint(0, 90));
}

static already_AddRefed<mozilla::layers::Layer>
CreateTestLayerTree(nsRefPtr<LayerManager>& aLayerManager, nsTArray<nsRefPtr<Layer> >& aLayers) {
  const char* layerTreeSyntax = "c(ttccc(c(c)))";
  
  nsIntRegion layerVisibleRegion[] = {
    nsIntRegion(nsIntRect(0,0,100,100)),
    nsIntRegion(nsIntRect(0,0,100,100)),
    nsIntRegion(nsIntRect(10,10,20,20)),
    nsIntRegion(nsIntRect(10,10,20,20)),
    nsIntRegion(nsIntRect(5,5,20,20)),
    nsIntRegion(nsIntRect(10,10,40,40)),
    nsIntRegion(nsIntRect(10,10,40,40)),
    nsIntRegion(nsIntRect(10,10,40,40)),
  };
  gfx3DMatrix transforms[] = {
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
    gfx3DMatrix(),
  };
  return CreateLayerTree(layerTreeSyntax, layerVisibleRegion, transforms, aLayerManager, aLayers);
}

static void
SetScrollableFrameMetrics(Layer* aLayer, FrameMetrics::ViewID aScrollId, MockContentController* mcc)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  FrameMetrics metrics;
  metrics.mScrollId = aScrollId;
  nsIntRect layerBound = aLayer->GetVisibleRegion().GetBounds();
  metrics.mCompositionBounds = ScreenIntRect(layerBound.x, layerBound.y,
                                             layerBound.width, layerBound.height);
  metrics.mViewport = CSSRect(layerBound.x, layerBound.y,
                              layerBound.width, layerBound.height);
  container->SetFrameMetrics(metrics);

  
  
  EXPECT_CALL(*mcc, SendAsyncScrollDOMEvent(_,_)).Times(1);
  EXPECT_CALL(*mcc, RequestContentRepaint(_)).Times(1);
}

TEST(APZCTreeManager, GetAPZCAtPoint) {
  nsTArray<nsRefPtr<Layer> > layers;
  nsRefPtr<LayerManager> lm;
  nsRefPtr<Layer> root = CreateTestLayerTree(lm, layers);

  TimeStamp testStartTime = TimeStamp::Now();
  AsyncPanZoomController::SetFrameTime(testStartTime);
  nsRefPtr<MockContentController> mcc = new MockContentController();
  ScopedLayerTreeRegistration controller(CompositorParent::ROOT_LAYER_TREE_ID, root, mcc);

  nsRefPtr<APZCTreeManager> manager = new TestAPZCTreeManager();

  
  nsRefPtr<AsyncPanZoomController> hit = manager->GetTargetAPZC(ScreenPoint(20, 20));
  AsyncPanZoomController* nullAPZC = nullptr;
  EXPECT_EQ(nullAPZC, hit.get());

  
  SetScrollableFrameMetrics(root, FrameMetrics::ROOT_SCROLL_ID, mcc);
  manager->UpdatePanZoomControllerTree(nullptr, root, CompositorParent::ROOT_LAYER_TREE_ID, false);
  hit = manager->GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(root->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  
  SetScrollableFrameMetrics(layers[3], FrameMetrics::START_SCROLL_ID, mcc);
  manager->UpdatePanZoomControllerTree(nullptr, root, CompositorParent::ROOT_LAYER_TREE_ID, false);
  EXPECT_NE(root->AsContainerLayer()->GetAsyncPanZoomController(), layers[3]->AsContainerLayer()->GetAsyncPanZoomController());
  hit = manager->GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  
  hit = manager->GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  SetScrollableFrameMetrics(layers[4], FrameMetrics::START_SCROLL_ID + 1, mcc);
  manager->UpdatePanZoomControllerTree(nullptr, root, CompositorParent::ROOT_LAYER_TREE_ID, false);
  hit = manager->GetTargetAPZC(ScreenPoint(15, 15));
  EXPECT_EQ(layers[4]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  
  hit = manager->GetTargetAPZC(ScreenPoint(90, 90));
  EXPECT_EQ(root->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  
  hit = manager->GetTargetAPZC(ScreenPoint(1000, 10));
  EXPECT_EQ(nullAPZC, hit.get());
  hit = manager->GetTargetAPZC(ScreenPoint(-1000, 10));
  EXPECT_EQ(nullAPZC, hit.get());

  
  gfx3DMatrix transform;
  transform.ScalePost(0.1, 0.1, 1);
  root->SetBaseTransform(transform);
  root->ComputeEffectiveTransforms(gfx3DMatrix());
  manager->UpdatePanZoomControllerTree(nullptr, root, CompositorParent::ROOT_LAYER_TREE_ID, false);
  hit = manager->GetTargetAPZC(ScreenPoint(50, 50)); 
  EXPECT_EQ(nullAPZC, hit.get());

  hit = manager->GetTargetAPZC(ScreenPoint(2, 2));
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  
  layers[4]->SetBaseTransform(transform);
  
  
  root->ComputeEffectiveTransforms(gfx3DMatrix());
  manager->UpdatePanZoomControllerTree(nullptr, root, CompositorParent::ROOT_LAYER_TREE_ID, false);
  hit = manager->GetTargetAPZC(ScreenPoint(2, 2));
  EXPECT_EQ(layers[3]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  
  SetScrollableFrameMetrics(layers[7], FrameMetrics::START_SCROLL_ID + 2, mcc);

  gfx3DMatrix translateTransform;
  translateTransform.Translate(gfxPoint3D(10, 10, 0));
  layers[5]->SetBaseTransform(translateTransform);

  gfx3DMatrix translateTransform2;
  translateTransform2.Translate(gfxPoint3D(-20, 0, 0));
  layers[6]->SetBaseTransform(translateTransform2);

  gfx3DMatrix translateTransform3;
  translateTransform3.ScalePost(1,15,1);
  layers[7]->SetBaseTransform(translateTransform3);

  root->ComputeEffectiveTransforms(gfx3DMatrix());
  manager->UpdatePanZoomControllerTree(nullptr, root, CompositorParent::ROOT_LAYER_TREE_ID, false);
  
  hit = manager->GetTargetAPZC(ScreenPoint(1, 45));
  EXPECT_EQ(layers[7]->AsContainerLayer()->GetAsyncPanZoomController(), hit.get());
  

  manager->ClearTree();
}


