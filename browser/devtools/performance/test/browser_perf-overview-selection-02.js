





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, OverviewView } = panel.panelWin;
  let graph = OverviewView.framerateGraph;

  yield startRecording(panel);

  ok(!graph.selectionEnabled,
    "Selection shouldn't be enabled when the first recording started.");

  yield stopRecording(panel);

  ok(graph.selectionEnabled,
    "Selection should be enabled when the first recording finishes.");

  yield startRecording(panel);

  ok(!graph.selectionEnabled,
    "Selection shouldn't be enabled when the second recording started.");

  yield stopRecording(panel);

  ok(graph.selectionEnabled,
    "Selection should be enabled when the first second finishes.");

  yield teardown(panel);
  finish();
}
