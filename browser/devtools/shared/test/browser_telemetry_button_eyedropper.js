


const TEST_URI = "data:text/html;charset=utf-8,<p>browser_telemetry_button_eyedropper.js</p><div>test</div>";

let promise = Cu.import("resource://gre/modules/devtools/deprecated-sync-thenables.js", {}).Promise;
let {Services} = Cu.import("resource://gre/modules/Services.jsm", {});

let require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
let Telemetry = require("devtools/shared/telemetry");

let { EyedropperManager } = require("devtools/eyedropper/eyedropper");


function init() {
  Telemetry.prototype.telemetryInfo = {};
  Telemetry.prototype._oldlog = Telemetry.prototype.log;
  Telemetry.prototype.log = function(histogramId, value) {
    if (!this.telemetryInfo) {
      
      return;
    }
    if (histogramId) {
      if (!this.telemetryInfo[histogramId]) {
        this.telemetryInfo[histogramId] = [];
      }

      this.telemetryInfo[histogramId].push(value);
    }
  };

  testButton("command-button-eyedropper");
}

function testButton(id) {
  info("Testing " + id);

  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
    info("inspector opened");

    let button = toolbox.doc.querySelector("#" + id);
    ok(button, "Captain, we have the button");

    
    button.click();

    checkResults("_EYEDROPPER_");
  }).then(null, console.error);
}

function clickButton(node, clicks) {
  for (let i = 0; i < clicks; i++) {
    info("Clicking button " + node.id);
    node.click();
  }
}

function checkResults(histIdFocus) {
  let result = Telemetry.prototype.telemetryInfo;

  for (let [histId, value] of Iterator(result)) {
    if (histId.startsWith("DEVTOOLS_INSPECTOR_") ||
        !histId.contains(histIdFocus)) {
      
      
      
      continue;
    }

    if (histId.endsWith("OPENED_PER_USER_FLAG")) {
      ok(value.length === 1 && value[0] === true,
         "Per user value " + histId + " has a single value of true");
    } else if (histId.endsWith("OPENED_BOOLEAN")) {
      ok(value.length == 1, histId + " has one entry");

      let okay = value.every(function(element) {
        return element === true;
      });

      ok(okay, "All " + histId + " entries are === true");
    }
  }

  finishUp();
}

function finishUp() {
  gBrowser.removeCurrentTab();

  Telemetry.prototype.log = Telemetry.prototype._oldlog;
  delete Telemetry.prototype._oldlog;
  delete Telemetry.prototype.telemetryInfo;

  TargetFactory = Services = promise = require = null;

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
