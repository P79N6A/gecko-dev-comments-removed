






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $ } = panel.panelWin;

  
  Services.prefs.setBoolPref(MEMORY_PREF, false);
  yield startRecording(panel);

  Services.prefs.setBoolPref(MEMORY_PREF, true);
  yield stopRecording(panel);

  is(PerformanceController.getCurrentRecording().getConfiguration().withMemory, false,
    "The recording finished without tracking memory.");
  is(PerformanceController.getCurrentRecording().getConfiguration().withAllocations, false,
    "The recording finished without tracking allocations.");

  
  yield startRecording(panel);
  Services.prefs.setBoolPref(MEMORY_PREF, false);
  yield stopRecording(panel);

  is(PerformanceController.getCurrentRecording().getConfiguration().withMemory, true,
    "The recording finished with tracking memory.");
  is(PerformanceController.getCurrentRecording().getConfiguration().withAllocations, true,
    "The recording finished with tracking allocations.");

  yield teardown(panel);
  finish();
}
