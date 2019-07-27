


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_storage.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  info("Activating the storage inspector");
  Services.prefs.setBoolPref("devtools.storage.enabled", true);

  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  yield openAndCloseToolbox(2, TOOL_DELAY, "storage");
  checkTelemetryResults(Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  gBrowser.removeCurrentTab();

  info("De-activating the storage inspector");
  Services.prefs.clearUserPref("devtools.storage.enabled");
});
