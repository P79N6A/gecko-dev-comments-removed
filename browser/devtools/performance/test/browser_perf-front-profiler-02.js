








let test = Task.async(function*() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let front = panel.panelWin.gFront;

  ok(!nsIProfilerModule.IsActive(),
    "The built-in profiler module should not have been automatically started.");

  let activated = front.once("profiler-activated");
  let rec = yield front.startRecording();
  yield activated;
  yield front.stopRecording(rec);
  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should still be active (1).");

  let alreadyActive = front.once("profiler-already-active");
  rec = yield front.startRecording();
  yield alreadyActive;
  yield front.stopRecording(rec);
  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should still be active (2).");

  yield teardown(panel);

  ok(!nsIProfilerModule.IsActive(),
    "The built-in profiler module should have been automatically stoped.");

  finish();
});
