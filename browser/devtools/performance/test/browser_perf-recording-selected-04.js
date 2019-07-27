







let test = Task.async(function*() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let { $, EVENTS, PerformanceController, RecordingsView } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel);

  yield startRecording(panel);
  yield stopRecording(panel);

  let rerender = waitForWidgetsRendered(panel);
  RecordingsView.selectedIndex = 0;
  yield rerender;

  rerender = waitForWidgetsRendered(panel);
  RecordingsView.selectedIndex = 1;
  yield rerender;

  yield teardown(panel);
  finish();
});
