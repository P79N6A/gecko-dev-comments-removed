






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $ } = panel.panelWin;

  Services.prefs.setBoolPref(FRAMERATE_PREF, false);
  ok($("#time-framerate").hidden, "fps graph is hidden when ticks disabled");

  yield startRecording(panel);
  yield stopRecording(panel);

  is(PerformanceController.getCurrentRecording().getConfiguration().withTicks, false,
    "PerformanceFront started without ticks recording.");

  Services.prefs.setBoolPref(FRAMERATE_PREF, true);
  ok(!$("#time-framerate").hidden, "fps graph is not hidden when ticks enabled");

  yield startRecording(panel);
  yield stopRecording(panel);

  is(PerformanceController.getCurrentRecording().getConfiguration().withTicks, true,
    "PerformanceFront started with ticks recording.");

  yield teardown(panel);
  finish();
}
