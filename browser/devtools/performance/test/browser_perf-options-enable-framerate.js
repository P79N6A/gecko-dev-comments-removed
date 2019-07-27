


const FRAMERATE_PREF = "devtools.performance.ui.enable-framerate";





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, $ } = panel.panelWin;

  let recordedWithTicks = null;

  let onStart = (_, recording, { withMemory, withTicks }) => {
    recordedWithTicks = withTicks;
  };
  PerformanceController.on(EVENTS.RECORDING_STARTED, onStart);

  Services.prefs.setBoolPref(FRAMERATE_PREF, false);
  ok($("#time-framerate").hidden, "fps graph is hidden when ticks disabled");

  yield startRecording(panel);
  yield stopRecording(panel);

  ok(recordedWithTicks === false, "PerformanceFront started without ticks recording.");

  Services.prefs.setBoolPref(FRAMERATE_PREF, true);
  ok(!$("#time-framerate").hidden, "fps graph is not hidden when ticks enabled");

  yield startRecording(panel);
  yield stopRecording(panel);

  ok(recordedWithTicks === true, "PerformanceFront started with ticks recording.");

  PerformanceController.off(EVENTS.RECORDING_STARTED, onStart);
  yield teardown(panel);
  finish();
}
