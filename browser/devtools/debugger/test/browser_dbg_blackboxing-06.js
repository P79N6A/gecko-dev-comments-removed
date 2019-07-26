







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

    waitForSourceAndCaretAndScopes(gPanel, ".html", 21)
      .then(testBlackBox)
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });

    gDebuggee.runTest();
  });
}

function testBlackBox() {
  const selectedUrl = gSources.selectedValue;

  let finished = waitForSourceShown(gPanel, "blackboxme.js").then(() => {
    const newSelectedUrl = gSources.selectedValue;
    isnot(selectedUrl, newSelectedUrl,
      "Should not have the same url selected.");

    return toggleBlackBoxing(gPanel).then(() => {
      is(gSources.selectedValue, newSelectedUrl,
        "The selected source did not change.");
    });
  });

  gSources.selectedIndex = 0;
  return finished;
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gSources = null;
});
