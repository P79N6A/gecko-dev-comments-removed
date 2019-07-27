


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_options.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);

  startTelemetry();

  yield openAndCloseToolbox(2, TOOL_DELAY, "options");
  checkResults();

  gBrowser.removeCurrentTab();
});

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_OPTIONS_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_OPTIONS_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_OPTIONS_TIME_ACTIVE_SECONDS", null, "hasentries");
}
