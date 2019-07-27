









let test = Task.async(function*() {
  
  let ENTRIES = 1000000;
  let INTERVAL = 1;
  let FEATURES = ["js"];
  nsIProfilerModule.StartProfiler(ENTRIES, INTERVAL, FEATURES, FEATURES.length);

  let [,, firstPanel] = yield initFrontend(SIMPLE_URL);
  let firstFront = firstPanel.panelWin.gFront;

  let alredyActive = firstFront.once("profiler-already-active");
  yield firstFront.startRecording();
  yield alredyActive;
  ok(firstFront._profilingStartTime > 0, "The profiler was not restarted.");

  let [,, secondPanel] = yield initFrontend(SIMPLE_URL);
  let secondFront = secondPanel.panelWin.gFront;

  let alreadyActive = secondFront.once("profiler-already-active");
  yield secondFront.startRecording();
  yield alreadyActive;
  ok(secondFront._profilingStartTime > 0, "The profiler was not restarted.");

  yield teardown(firstPanel);
  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should still be active.");

  yield teardown(secondPanel);
  ok(!nsIProfilerModule.IsActive(),
    "The built-in profiler module should have been automatically stoped.");

  finish();
});
