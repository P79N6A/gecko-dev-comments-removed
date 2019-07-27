





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, FlameGraphView } = panel.panelWin;

  yield startRecording(panel);
  yield waitUntil(() => PerformanceController.getCurrentRecording().getMarkers().length);

  let rendered = once(FlameGraphView, EVENTS.FLAMEGRAPH_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "FlameGraphView rendered after recording is stopped.");

  yield teardown(panel);
  finish();
}
