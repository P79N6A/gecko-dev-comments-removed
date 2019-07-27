






function* spawnTest() {
  let { target, panel } = yield initPerformance(SIMPLE_URL);
  let { $, $$, EVENTS, PerformanceController, OverviewView, WaterfallView } = panel.panelWin;

  yield startRecording(panel);
  ok(true, "Recording has started.");

  let updated = 0;
  OverviewView.on(EVENTS.OVERVIEW_RENDERED, () => updated++);

  ok((yield waitUntil(() => updated > 0)),
    "The overview graphs were updated a bunch of times.");
  ok((yield waitUntil(() => PerformanceController.getCurrentRecording().getMarkers().length > 0)),
    "There are some markers available.");

  yield stopRecording(panel);
  ok(true, "Recording has ended.");

  let initialBarsCount = $$(".waterfall-marker-bar").length;

  
  let timeline = OverviewView.graphs.get("timeline");
  let rerendered = WaterfallView.once(EVENTS.WATERFALL_RENDERED);
  timeline.setSelection({ start: 0, end: timeline.width / 2 })
  yield rerendered;

  let beforeResizeBarsCount = $$(".waterfall-marker-bar").length;
  ok(beforeResizeBarsCount < initialBarsCount,
    "A subset of the total markers was selected.");

  rerendered = WaterfallView.once(EVENTS.WATERFALL_RENDERED);
  EventUtils.sendMouseEvent({ type: "mouseup" }, WaterfallView.detailsSplitter);
  yield rerendered;

  let afterResizeBarsCount = $$(".waterfall-marker-bar").length;
  is(afterResizeBarsCount, beforeResizeBarsCount,
    "The same subset of the total markers remained visible.");

  yield teardown(panel);
  finish();
}
