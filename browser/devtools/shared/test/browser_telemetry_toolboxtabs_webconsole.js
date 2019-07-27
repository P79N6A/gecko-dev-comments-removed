


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_styleeditor_webconsole.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  yield openAndCloseToolbox(2, TOOL_DELAY, "webconsole");
  checkTelemetryResults(Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  gBrowser.removeCurrentTab();
});
