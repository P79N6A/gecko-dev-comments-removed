









let test = Task.async(function*() {
  
  let ENTRIES = 1000000;
  let INTERVAL = 1;
  let FEATURES = ["js"];
  nsIProfilerModule.StartProfiler(ENTRIES, INTERVAL, FEATURES, FEATURES.length);

  let { panel: firstPanel } = yield initPerformance(SIMPLE_URL);
  let firstFront = firstPanel.panelWin.gFront;

  let firstAlreadyActive = firstFront.once("profiler-already-active");
  let recording = yield firstFront.startRecording();
  yield firstAlreadyActive;
  ok(recording._profilerStartTime > 0, "The profiler was not restarted.");

  let { panel: secondPanel } = yield initPerformance(SIMPLE_URL);
  let secondFront = secondPanel.panelWin.gFront;

  let secondAlreadyActive = secondFront.once("profiler-already-active");
  let secondRecording = yield secondFront.startRecording();
  yield secondAlreadyActive;
  ok(secondRecording._profilerStartTime > 0, "The profiler was not restarted.");

  yield teardown(firstPanel);
  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should still be active.");

  yield teardown(secondPanel);
  ok(!nsIProfilerModule.IsActive(),
    "The built-in profiler module should have been automatically stoped.");

  finish();
});
