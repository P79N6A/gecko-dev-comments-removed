





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, OverviewView, DetailsView, WaterfallView, JsFlameGraphView } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel);

  
  WaterfallView.rangeChangeDebounceTime = 0;
  JsFlameGraphView.rangeChangeDebounceTime = 0;

  yield DetailsView.selectView("js-flamegraph");
  let duration = PerformanceController.getCurrentRecording().getDuration();

  
  Object.defineProperty(OverviewView, "isMouseActive", { value: true });

  
  let flamegraphRendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  OverviewView.emit(EVENTS.OVERVIEW_RANGE_SELECTED, { startTime: 0, endTime: duration });
  yield flamegraphRendered;
  ok(true, "FlameGraph rerenders when mouse is active (1)");

  flamegraphRendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  OverviewView.emit(EVENTS.OVERVIEW_RANGE_SELECTED, { startTime: 0, endTime: duration });
  yield flamegraphRendered;
  ok(true, "FlameGraph rerenders when mouse is active (2)");

  ok(OverviewView.isMouseActive, "Fake mouse is still active");

  
  Object.defineProperty(OverviewView, "isMouseActive", { value: false });
  yield DetailsView.selectView("waterfall");

  
  Object.defineProperty(OverviewView, "isMouseActive", { value: true });

  let oneSecondElapsed = false;
  let waterfallRendered = false;

  WaterfallView.on(EVENTS.WATERFALL_RENDERED, () => waterfallRendered = true);

  
  idleWait(1000).then(() => oneSecondElapsed = true);
  yield waitUntil(() => {
    OverviewView.emit(EVENTS.OVERVIEW_RANGE_SELECTED, { startTime: 0, endTime: duration });
    return oneSecondElapsed;
  });

  ok(OverviewView.isMouseActive, "Fake mouse is still active");
  ok(!waterfallRendered, "the waterfall view should not have been rendered while mouse is active.");

  yield teardown(panel);
  finish();
}
