






const TAB_URL = EXAMPLE_URL + "doc_blackboxing.html";

let gTab, gDebuggee, gPanel, gDebugger;
let gSources;

function test() {
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gSources = gDebugger.DebuggerView.Sources;

    waitForSourceShown(gPanel, ".js")
      .then(testBlackBox)
      .then(() => closeDebuggerAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function testBlackBox() {
  const selectedUrl = gSources.selectedValue;
  const checkbox = getDifferentBlackBoxCheckbox(selectedUrl);
  ok(checkbox, "We should be able to grab a different checkbox.");

  let finished = waitForThreadEvents(gPanel, "blackboxchange").then(() => {
    is(selectedUrl, gSources.selectedValue,
      "The same source should still be selected.");
  });

  checkbox.click();
  return finished;
}

function getDifferentBlackBoxCheckbox(aUrl) {
  return gDebugger.document.querySelector(
    ".side-menu-widget-item:not([tooltiptext=\"" + aUrl + "\"]) " +
    ".side-menu-widget-item-checkbox");
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gSources = null;
});
