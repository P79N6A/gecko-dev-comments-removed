


const TEST_URI = "data:text/html;charset=utf-8," +
  "<p>browser_telemetry_button_scratchpad.js</p>";



const TOOL_DELAY = 200;

add_task(function*() {
  yield promiseTab(TEST_URI);

  startTelemetry();

  let target = TargetFactory.forTab(gBrowser.selectedTab);
  let toolbox = yield gDevTools.showToolbox(target, "inspector");
  info("inspector opened");

  let onAllWindowsOpened = trackScratchpadWindows();

  info("testing the scratchpad button");
  yield testButton(toolbox);
  yield onAllWindowsOpened;

  checkResults();

  yield gDevTools.closeToolbox(target);
  gBrowser.removeCurrentTab();
});

function trackScratchpadWindows() {
  info("register the window observer to track when scratchpad windows open");

  let numScratchpads = 0;

  return new Promise(resolve => {
    Services.ww.registerNotification(function observer(subject, topic) {
      if (topic == "domwindowopened") {
        let win = subject.QueryInterface(Ci.nsIDOMWindow);
        win.addEventListener("load", function onLoad() {
          win.removeEventListener("load", onLoad, false);

          if (win.Scratchpad) {
            win.Scratchpad.addObserver({
              onReady: function() {
                win.Scratchpad.removeObserver(this);
                numScratchpads++;
                win.close();

                info("another scratchpad was opened and closed, count is now " + numScratchpads);

                if (numScratchpads === 4) {
                  Services.ww.unregisterNotification(observer);
                  info("4 scratchpads have been opened and closed, checking results");
                  resolve();
                }
              },
            });
          }
        }, false);
      }
    });
  });
}

function* testButton(toolbox) {
  info("Testing command-button-scratchpad");
  let button = toolbox.doc.querySelector("#command-button-scratchpad");
  ok(button, "Captain, we have the button");

  yield delayedClicks(button, 4);
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

function checkResults(histIdFocus) {
  
  
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_LISTTABS_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_PROTOCOLDESCRIPTION_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_DEBUGGER_RDP_LOCAL_RECONFIGURETAB_MS", null, "hasentries");
  checkTelemetry("DEVTOOLS_INSPECTOR_OPENED_BOOLEAN", [0,1,0]);
  checkTelemetry("DEVTOOLS_INSPECTOR_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_RULEVIEW_OPENED_BOOLEAN", [0,1,0]);
  checkTelemetry("DEVTOOLS_RULEVIEW_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_SCRATCHPAD_OPENED_BOOLEAN", [0,4,0]);
  checkTelemetry("DEVTOOLS_SCRATCHPAD_OPENED_PER_USER_FLAG", [0,1,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_BOOLEAN", [0,1,0]);
  checkTelemetry("DEVTOOLS_TOOLBOX_OPENED_PER_USER_FLAG", [0,1,0]);
}
