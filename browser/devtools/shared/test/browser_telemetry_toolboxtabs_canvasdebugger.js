


const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>browser_telemetry_toolboxtabs_canvasdebugger.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  info("Activate the canvasdebugger");
  let originalPref = Services.prefs.getBoolPref("devtools.canvasdebugger.enabled");
  Services.prefs.setBoolPref("devtools.canvasdebugger.enabled", true);

  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  yield openAndCloseToolbox(2, TOOL_DELAY, "canvasdebugger");
  checkTelemetryResults(Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  gBrowser.removeCurrentTab();

  info("De-activate the canvasdebugger");
  Services.prefs.setBoolPref("devtools.canvasdebugger.enabled", originalPref);
});
