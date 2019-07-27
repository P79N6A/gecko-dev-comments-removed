







thisTestLeaksUncaughtRejectionsAndShouldBeFixed("Error: Shader Editor is still waiting for a WebGL context to be created.");

const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_shadereditor.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  info("Active the sharer editor");
  let originalPref = Services.prefs.getBoolPref("devtools.shadereditor.enabled");
  Services.prefs.setBoolPref("devtools.shadereditor.enabled", true);

  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  yield openAndCloseToolbox(2, TOOL_DELAY, "shadereditor");
  checkTelemetryResults(Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  gBrowser.removeCurrentTab();

  info("De-activate the sharer editor");
  Services.prefs.setBoolPref("devtools.shadereditor.enabled", originalPref);
});
