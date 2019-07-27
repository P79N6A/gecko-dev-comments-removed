


"use strict";

const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>browser_telemetry_button_paintflashing.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);
  let Telemetry = loadTelemetryAndRecordLogs();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = yield gDevTools.showToolbox(target, "inspector");
  info("inspector opened");

  info("testing the paintflashing button");
  yield testButton(toolbox, Telemetry);

  stopRecordingTelemetryLogs(Telemetry);
  yield gDevTools.closeToolbox(target);
  gBrowser.removeCurrentTab();
});

function* testButton(toolbox, Telemetry) {
  info("Testing command-button-paintflashing");

  let button = toolbox.doc.querySelector("#command-button-paintflashing");
  ok(button, "Captain, we have the button");

  yield* delayedClicks(toolbox, button, 4);
  checkResults("_PAINTFLASHING_", Telemetry);
}

function* delayedClicks(toolbox, node, clicks) {
  for (let i = 0; i < clicks; i++) {
    yield new Promise(resolve => {
      
      setTimeout(() => resolve(), TOOL_DELAY);
    });

    
    
    let clicked = toolbox._requisition.commandOutputManager.onOutput.once();

    info("Clicking button " + node.id);
    node.click();

    let outputEvent = yield clicked;
    
    yield outputEvent.output.promise;
  }
}

function checkResults(histIdFocus, Telemetry) {
  let result = Telemetry.prototype.telemetryInfo;

  for (let [histId, value] of Iterator(result)) {
    if (histId.startsWith("DEVTOOLS_INSPECTOR_") ||
        !histId.includes(histIdFocus)) {
      
      
      
      continue;
    }

    if (histId.endsWith("OPENED_PER_USER_FLAG")) {
      ok(value.length === 1 && value[0] === true,
         "Per user value " + histId + " has a single value of true");
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
