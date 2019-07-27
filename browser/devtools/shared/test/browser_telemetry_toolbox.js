


const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>browser_telemetry_toolbox.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  yield openAndCloseToolbox(3, TOOL_DELAY, "inspector");
  checkTelemetryResults(Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  gBrowser.removeCurrentTab();
});
