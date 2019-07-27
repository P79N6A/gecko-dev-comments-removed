


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_sidebar.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = yield gDevTools.showToolbox(target, "inspector");
  info("inspector opened");

  yield testSidebar(toolbox);
  checkResults(Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  yield gDevTools.closeToolbox(target);
  gBrowser.removeCurrentTab();
});

function* testSidebar(toolbox) {
  info("Testing sidebar");

  let inspector = toolbox.getCurrentPanel();
  let sidebarTools = ["ruleview", "computedview", "fontinspector",
                      "layoutview", "animationinspector"];

  
  sidebarTools.push.apply(sidebarTools, sidebarTools);

  return new Promise(resolve => {
    
    setTimeout(function selectSidebarTab() {
      let tool = sidebarTools.pop();
      if (tool) {
        inspector.sidebar.select(tool);
        setTimeout(function() {
          setTimeout(selectSidebarTab, TOOL_DELAY);
        }, TOOL_DELAY);
      } else {
        resolve();
      }
    }, TOOL_DELAY);
  });
}

function checkResults(Telemetry) {
  let result = Telemetry.prototype.telemetryInfo;

  for (let [histId, value] of Iterator(result)) {
    if (histId.startsWith("DEVTOOLS_INSPECTOR_")) {
      
      
      continue;
    }

    if (histId.endsWith("OPENED_PER_USER_FLAG")) {
      ok(value.length === 1 && value[0] === true,
         "Per user value " + histId + " has a single value of true");
    } else if (histId === "DEVTOOLS_TOOLBOX_OPENED_BOOLEAN") {
      is(value.length, 1, histId + " has only one entry");
    } else if (histId.endsWith("OPENED_BOOLEAN")) {
      ok(value.length > 1, histId + " has more than one entry");

      let okay = value.every(function(element) {
        return element === true;
      });

      ok(okay, "All " + histId + " entries are === true");
    } else if (histId.endsWith("TIME_ACTIVE_SECONDS")) {
      ok(value.length > 1, histId + " has more than one entry");

      let okay = value.every(function(element) {
        return element > 0;
      });

      ok(okay, "All " + histId + " entries have time > 0");
    }
  }
}
