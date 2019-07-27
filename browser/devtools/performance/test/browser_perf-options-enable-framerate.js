






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $ } = panel.panelWin;
  Services.prefs.setBoolPref(FRAMERATE_PREF, false);

  yield startRecording(panel);
  yield stopRecording(panel);

  is(PerformanceController.getCurrentRecording().getConfiguration().withTicks, false,
    "PerformanceFront started without ticks recording.");
  ok($("#time-framerate").hidden, "fps graph is hidden when ticks disabled");

  Services.prefs.setBoolPref(FRAMERATE_PREF, true);
  ok($("#time-framerate").hidden, "fps graph is still hidden if recording does not contain ticks.");

  yield startRecording(panel);
  yield stopRecording(panel);

  ok(!$("#time-framerate").hidden, "fps graph is not hidden when ticks enabled before recording");
  is(PerformanceController.getCurrentRecording().getConfiguration().withTicks, true,
    "PerformanceFront started with ticks recording.");

  yield teardown(panel);
  finish();
}
