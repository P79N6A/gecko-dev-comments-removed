


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
  
  
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_LISTTABS_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_RECONFIGURETAB_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_TABDETACH_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_OPTIONS_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_OPTIONS_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_OPTIONS_TIME_ACTIVE_SECONDS", null, "hasentries");
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_TIME_ACTIVE_SECONDS", null, "hasentries");
}
