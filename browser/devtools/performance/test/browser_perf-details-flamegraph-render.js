





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, FlameGraphView } = panel.panelWin;

  DetailsView.selectView("flamegraph");
  ok(DetailsView.isViewSelected(FlameGraphView), "The flamegraph is now selected.");

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(FlameGraphView, EVENTS.FLAMEGRAPH_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "FlameGraphView rendered after recording is stopped.");

  yield startRecording(panel);
  yield busyWait(100);

  rendered = once(FlameGraphView, EVENTS.FLAMEGRAPH_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "FlameGraphView rendered again after recording completed a second time.");

  yield teardown(panel);
  finish();
}
