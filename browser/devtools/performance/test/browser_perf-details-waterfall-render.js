





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, WaterfallView } = panel.panelWin;

  yield startRecording(panel);
  yield waitUntil(() => PerformanceController.getCurrentRecording().getMarkers().length);

  let rendered = once(WaterfallView, EVENTS.WATERFALL_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "WaterfallView rendered after recording is stopped.");

  yield startRecording(panel);
  yield waitUntil(() => PerformanceController.getCurrentRecording().getMarkers().length);

  rendered = once(WaterfallView, EVENTS.WATERFALL_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "WaterfallView rendered again after recording completed a second time.");

  yield teardown(panel);
  finish();
}
