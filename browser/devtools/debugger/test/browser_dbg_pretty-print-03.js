






const TAB_URL = EXAMPLE_URL + "doc_pretty-print.html";

let gTab, gDebuggee, gPanel, gDebugger;

function test() {
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;

    waitForSourceShown(gPanel, "code_ugly.js")
      .then(runCodeAndPause)
      .then(() => {
        const sourceShown = waitForSourceShown(gPanel, "code_ugly.js");
        const caretUpdated = waitForCaretUpdated(gPanel, 7);
        const finished = promise.all([sourceShown, caretUpdated]);
        clickPrettyPrintButton();
        return finished;
      })
      .then(resumeDebuggerThenCloseAndFinish.bind(null, gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + DevToolsUtils.safeErrorString(aError));
      });
  });
}

function runCodeAndPause() {
  const deferred = promise.defer();
  once(gDebugger.gThreadClient, "paused").then(deferred.resolve);
  
  executeSoon(gDebuggee.foo);
  return deferred.promise;
}

function clickPrettyPrintButton() {
  EventUtils.sendMouseEvent({ type: "click" },
                            gDebugger.document.getElementById("pretty-print"),
                            gDebugger);
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
});
