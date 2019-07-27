


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_webaudioeditor.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  info("Activating the webaudioeditor");
  let originalPref = Services.prefs.getBoolPref("devtools.webaudioeditor.enabled");
  Services.prefs.setBoolPref("devtools.webaudioeditor.enabled", true);

  yield promiseTab(TEST_URI);

  startTelemetry();

  yield openAndCloseToolbox(2, TOOL_DELAY, "webaudioeditor");
  checkResults();

  gBrowser.removeCurrentTab();

  info("De-activating the webaudioeditor");
  Services.prefs.setBoolPref("devtools.webaudioeditor.enabled", originalPref);
});

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_WEBAUDIOEDITOR_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_WEBAUDIOEDITOR_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_WEBAUDIOEDITOR_TIME_ACTIVE_SECONDS", null, "hasentries");
}
