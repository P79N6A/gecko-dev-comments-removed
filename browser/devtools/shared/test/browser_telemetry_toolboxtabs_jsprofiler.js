


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_jsprofiler.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);
  startTelemetry();

  yield openAndCloseToolbox(2, TOOL_DELAY, "performance");
  checkResults();

  gBrowser.removeCurrentTab();
});

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_JSPROFILER_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_JSPROFILER_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_JSPROFILER_TIME_ACTIVE_SECONDS", null, "hasentries");
}
