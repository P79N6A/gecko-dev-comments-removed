







const TAB_URL = EXAMPLE_URL + "doc_with-frame.html";

let gTab, gPanel, gDebugger;
let gBreakpoints, gSources, gVariables;

function test() {
  
  requestLongerTimeout(4);

  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gBreakpoints = gDebugger.DebuggerController.Breakpoints;
    gSources = gDebugger.DebuggerView.Sources;
    gVariables = gDebugger.DebuggerView.Variables;
    let bubble = gDebugger.DebuggerView.VariableBubble;
    let tooltip = bubble._tooltip.panel;

    waitForSourceShown(gPanel, ".html")
      .then(addBreakpoint)
      .then(() => ensureThreadClientState(gPanel, "resumed"))
      .then(pauseDebuggee)
      .then(() => openVarPopup(gPanel, { line: 20, ch: 17 }))
      .then(function(){
        is(tooltip.querySelectorAll(".devtools-tooltip-simple-text").length, 1,
          "The popup should be open with a simple text entry");
        return stepInDebuggee(gDebugger, gPanel);
      })
      .then(() => checkVariablePopupClosed(bubble, tooltip))
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });
}

function addBreakpoint() {
  return gBreakpoints.addBreakpoint({ actor: gSources.selectedValue, line: 21 });
}

function pauseDebuggee() {
  sendMouseClickToTab(gTab, content.document.querySelector("button"));

  
  
  return promise.all([
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_SCOPES),
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_VARIABLES)
  ]);
}

function stepInDebuggee(gDebugger, gPanel) {
  
  
  executeSoon(() => {
    EventUtils.sendMouseEvent({ type: "mousedown" },
      gDebugger.document.querySelector("#step-in"),
      gDebugger);
  });

  return promise.all([
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_SCOPES, 1),
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_VARIABLES, 1)
  ]);
}

function checkVariablePopupClosed(bubble){
  ok(!bubble.contentsShown(),
    "When stepping, popup should close and be hidden.");
  ok(bubble._tooltip.isEmpty(),
    "The variable inspection popup should now be empty.");
  ok(!bubble._markedText,
    "The marked text in the editor was removed.");

  return promise.resolve(null);
}



registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
  gBreakpoints = null;
  gSources = null;
  gVariables = null;
});
