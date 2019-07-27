







thisTestLeaksUncaughtRejectionsAndShouldBeFixed("Error: Shader Editor is still waiting for a WebGL context to be created.");

const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_shadereditor.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  info("Active the sharer editor");
  let originalPref = Services.prefs.getBoolPref("devtools.shadereditor.enabled");
  Services.prefs.setBoolPref("devtools.shadereditor.enabled", true);

  yield promiseTab(TEST_URI);

  startTelemetry();

  yield openAndCloseToolbox(2, TOOL_DELAY, "shadereditor");
  checkResults();

  gBrowser.removeCurrentTab();

  info("De-activate the sharer editor");
  Services.prefs.setBoolPref("devtools.shadereditor.enabled", originalPref);
});

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_LISTTABS_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_RECONFIGURETAB_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_TABDETACH_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_SHADEREDITOR_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_SHADEREDITOR_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_SHADEREDITOR_TIME_ACTIVE_SECONDS", null, "hasentries");
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_TIME_ACTIVE_SECONDS", null, "hasentries");
}
