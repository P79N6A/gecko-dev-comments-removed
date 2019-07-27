


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_toolboxtabs_styleeditor_webconsole.js</p>";



const TOOL_DELAY = 200;

let {Promise: promise} = Cu.import("resource://gre/modules/devtools/deprecated-sync-thenables.js", {});

function init() {
  startTelemetry();
  openToolboxTabTwice("webconsole", false);
}

function openToolboxTabTwice(id, secondPass) {
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gDevTools.showToolbox(target, id).then(function(toolbox) {
    info("Toolbox tab " + id + " opened");

    toolbox.once("destroyed", function() {
      if (secondPass) {
        checkResults();
      } else {
        openToolboxTabTwice(id, true);
      }
    });
    
    setTimeout(function() {
      gDevTools.closeToolbox(target);
    }, TOOL_DELAY);
  }).then(null, reportError);
}

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_LISTTABS_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_RECONFIGURETAB_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_TABDETACH_MS", null, "hasentries");

  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_TIME_ACTIVE_SECONDS", null, "hasentries");

  checkTelemetry("DEVTOOLS_WEBCONSOLE_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_WEBCONSOLE_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_WEBCONSOLE_TIME_ACTIVE_SECONDS", null, "hasentries");

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
