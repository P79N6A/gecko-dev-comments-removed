






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

  

  ok($(".waterfall-header-container"),
    "A header container should have been created.");

  

  ok($(".waterfall-header-container > .waterfall-sidebar"),
    "A header sidebar node should have been created.");
  ok($(".waterfall-header-container > .waterfall-sidebar > .waterfall-header-name"),
    "A header name label should have been created inside the sidebar.");

  

  ok($(".waterfall-header-ticks"),
    "A header ticks node should have been created.");
  ok($$(".waterfall-header-ticks > .waterfall-header-tick").length > 0,
    "Some header tick labels should have been created inside the tick node.");

  

  ok($$(".waterfall-tree-item > .waterfall-sidebar").length,
    "Some marker sidebar nodes should have been created.");
  ok($$(".waterfall-tree-item > .waterfall-sidebar > .waterfall-marker-bullet").length,
    "Some marker color bullets should have been created inside the sidebar.");
  ok($$(".waterfall-tree-item > .waterfall-sidebar > .waterfall-marker-name").length,
    "Some marker name labels should have been created inside the sidebar.");

  

  ok($$(".waterfall-tree-item > .waterfall-marker").length,
    "Some marker waterfall nodes should have been created.");
  ok($$(".waterfall-tree-item > .waterfall-marker > .waterfall-marker-bar").length,
    "Some marker color bars should have been created inside the waterfall.");

  yield teardown(panel);
  finish();
}
