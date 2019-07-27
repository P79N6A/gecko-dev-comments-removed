






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $ } = panel.panelWin;

  Services.prefs.setBoolPref(MEMORY_PREF, false);

  yield startRecording(panel);
  yield stopRecording(panel);

  is(PerformanceController.getCurrentRecording().getConfiguration().withMemory, false,
    "PerformanceFront started without memory recording.");
  is(PerformanceController.getCurrentRecording().getConfiguration().withAllocations, false,
    "PerformanceFront started without allocations recording.");
  ok($("#memory-overview").hidden, "memory graph is hidden when memory disabled");

  Services.prefs.setBoolPref(MEMORY_PREF, true);
  ok($("#memory-overview").hidden,
    "memory graph is still hidden after enabling if recording did not start recording memory");

  yield startRecording(panel);
  yield stopRecording(panel);

  ok(!$("#memory-overview").hidden, "memory graph is not hidden when memory enabled before recording");
  is(PerformanceController.getCurrentRecording().getConfiguration().withMemory, true,
    "PerformanceFront started with memory recording.");
  is(PerformanceController.getCurrentRecording().getConfiguration().withAllocations, true,
    "PerformanceFront started with allocations recording.");

  yield teardown(panel);
  finish();
}
