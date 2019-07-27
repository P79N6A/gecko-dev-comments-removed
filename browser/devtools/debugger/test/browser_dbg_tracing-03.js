






const TAB_URL = EXAMPLE_URL + "doc_tracing-01.html";

let gTab, gPanel, gDebugger;

function test() {
  SpecialPowers.pushPrefEnv({'set': [["devtools.debugger.tracer", true]]}, () => {
    initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
      gTab = aTab;
      gPanel = aPanel;
      gDebugger = gPanel.panelWin;

      waitForSourceShown(gPanel, "code_tracing-01.js")
        .then(() => startTracing(gPanel))
        .then(clickButton)
        .then(() => waitForClientEvents(aPanel, "traces"))
        .then(() => {
          
          
          aPanel.panelWin.DebuggerView.Sources.selectedValue = TAB_URL;
          return ensureSourceIs(aPanel, TAB_URL, true);
        })
        .then(() => {
          const finished = waitForSourceShown(gPanel, "code_tracing-01.js");
          clickTraceLog();
          return finished;
        })
        .then(testCorrectLine)
        .then(() => stopTracing(gPanel))
        .then(() => {
          const deferred = promise.defer();
          SpecialPowers.popPrefEnv(deferred.resolve);
          return deferred.promise;
        })
        .then(() => closeDebuggerAndFinish(gPanel))
        .then(null, aError => {
          ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
        });
    });
  });
}

function clickButton() {
  sendMouseClickToTab(gTab, content.document.querySelector("button"));
}

function clickTraceLog() {
  filterTraces(gPanel, t => t.querySelector(".trace-name[value=main]"))[0].click();
}

function testCorrectLine() {
  is(gDebugger.DebuggerView.editor.getCursor().line, 18,
     "The editor should have the function definition site's line selected.");
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
});
