






let WAIT_TIME = 10;

function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController } = panel.panelWin;
  let front = panel.panelWin.gFront;

  ok(!nsIProfilerModule.IsActive(),
    "The built-in profiler module should not have been automatically started.");

  yield startRecording(panel);
  busyWait(WAIT_TIME); 

  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should now be active.");

  yield stopRecording(panel);

  ok(nsIProfilerModule.IsActive(),
    "The built-in profiler module should still be active.");

  yield teardown(panel);
  finish();
}
