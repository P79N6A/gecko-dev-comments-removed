


const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>browser_telemetry_button_tilt.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);

  startTelemetry();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = yield gDevTools.showToolbox(target, "inspector");
  info("inspector opened");

  info("testing the tilt button");
  yield testButton(toolbox);

  yield gDevTools.closeToolbox(target);
  gBrowser.removeCurrentTab();
});

function* testButton(toolbox) {
  info("Testing command-button-tilt");

  let button = toolbox.doc.querySelector("#command-button-tilt");
  ok(button, "Captain, we have the button");

  yield delayedClicks(button, 4);
  checkResults();
}

function delayedClicks(node, clicks) {
  return new Promise(resolve => {
    let clicked = 0;

    
    setTimeout(function delayedClick() {
      info("Clicking button " + node.id);
      node.click();
      clicked++;

      if (clicked >= clicks) {
        resolve(node);
      } else {
        setTimeout(delayedClick, TOOL_DELAY);
      }
    }, TOOL_DELAY);
  });
}

function checkResults() {
  
  
  checkTelemetry("DEVTOOLS_TILT_OPENED_BOOLEAN", [0,2,0]);
  checkTelemetry("DEVTOOLS_TILT_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_TILT_TIME_ACTIVE_SECONDS", null, "hasentries");
}
