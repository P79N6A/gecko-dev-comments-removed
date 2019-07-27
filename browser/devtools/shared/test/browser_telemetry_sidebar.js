


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_sidebar.js</p>";



const TOOL_DELAY = 200;

let {Promise: promise} = Cu.import("resource://gre/modules/devtools/deprecated-sync-thenables.js", {});

function init() {
  startTelemetry();
  testSidebar();
}

function testSidebar() {
  info("Testing sidebar");

  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    let inspector = toolbox.getCurrentPanel();
    let sidebarTools = ["ruleview", "computedview", "fontinspector", "layoutview"];

    
    sidebarTools.push.apply(sidebarTools, sidebarTools);

    
    setTimeout(function selectSidebarTab() {
      let tool = sidebarTools.pop();
      if (tool) {
        inspector.sidebar.select(tool);
        setTimeout(function() {
          setTimeout(selectSidebarTab, TOOL_DELAY);
        }, TOOL_DELAY);
      } else {
        checkResults();
      }
    }, TOOL_DELAY);
  });
}

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_COMPUTEDVIEW_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_COMPUTEDVIEW_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_COMPUTEDVIEW_TIME_ACTIVE_SECONDS", null, "hasentries");

  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_LISTTABS_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_RECONFIGURETAB_MS", null, "hasentries");

  checkTelemetry("DEVTOOLS_FONTINSPECTOR_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_FONTINSPECTOR_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_FONTINSPECTOR_TIME_ACTIVE_SECONDS", null, "hasentries");

  checkTelemetry("DEVTOOLS_INSPECTOR_OPENED_BOOLEAN", [0,1,0]);
  checkTelemetry("DEVTOOLS_INSPECTOR_OPENED_PER_USER_FLAG", [0,1,0]);

  checkTelemetry("DEVTOOLS_LAYOUTVIEW_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_LAYOUTVIEW_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_LAYOUTVIEW_TIME_ACTIVE_SECONDS", null, "hasentries");

  checkTelemetry("DEVTOOLS_RULEVIEW_OPENED_BOOLEAN", [0,3,0]);
  checkTelemetry("DEVTOOLS_RULEVIEW_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_RULEVIEW_TIME_ACTIVE_SECONDS", null, "hasentries");

  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_BOOLEAN", [0,1,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_PER_USER_FLAG", [0,1,0]);

  finishUp();
}

function finishUp() {
  gBrowser.removeCurrentTab();

  TargetFactory = promise = null;

  finish();
}

function test() {
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    waitForFocus(init, content);
  }, true);

  content.location = TEST_URI;
}
