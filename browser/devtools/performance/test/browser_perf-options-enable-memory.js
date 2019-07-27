


const MEMORY_PREF = "devtools.performance.ui.enable-memory";





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $ } = panel.panelWin;

  let recordedWithMemory = null;

  let onStart = (_, recording, { withMemory, withTicks }) => {
    recordedWithMemory = withMemory;
  };
  PerformanceController.on(EVENTS.RECORDING_STARTED, onStart);

  Services.prefs.setBoolPref(MEMORY_PREF, false);
  ok($("#memory-overview").hidden, "memory graph is hidden when memory disabled");

  yield startRecording(panel);
  yield stopRecording(panel);

  ok(recordedWithMemory === false, "PerformanceFront started without memory recording.");

  Services.prefs.setBoolPref(MEMORY_PREF, true);
  ok(!$("#memory-overview").hidden, "memory graph is not hidden when memory enabled");

  yield startRecording(panel);
  yield stopRecording(panel);

  ok(recordedWithMemory === true, "PerformanceFront started with memory recording.");

  PerformanceController.off(EVENTS.RECORDING_STARTED, onStart);
  yield teardown(panel);
  finish();
}
