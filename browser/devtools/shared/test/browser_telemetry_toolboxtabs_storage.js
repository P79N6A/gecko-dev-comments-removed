


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_storage.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  info("Activating the storage inspector");
  Services.prefs.setBoolPref("devtools.storage.enabled", true);

  yield promiseTab(TEST_URI);

  startTelemetry();

  yield openAndCloseToolbox(2, TOOL_DELAY, "storage");
  checkResults();

  gBrowser.removeCurrentTab();

  info("De-activating the storage inspector");
  Services.prefs.clearUserPref("devtools.storage.enabled");
});

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_STORAGE_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_STORAGE_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_STORAGE_TIME_ACTIVE_SECONDS", null, "hasentries");
}
