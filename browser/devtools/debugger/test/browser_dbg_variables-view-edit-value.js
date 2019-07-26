






const TAB_URL = EXAMPLE_URL + "doc_frame-parameters.html";

let gTab, gDebuggee, gPanel, gDebugger;
let gVars;

function test() {
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gVars = gDebugger.DebuggerView.Variables;

    waitForSourceAndCaretAndScopes(gPanel, ".html", 24)
      .then(() => initialChecks())
      .then(() => testModification("a", "1"))
      .then(() => testModification("{ a: 1 }", "Object"))
      .then(() => testModification("[a]", "Array"))
      .then(() => testModification("b", "Object"))
      .then(() => testModification("b.a", "1"))
      .then(() => testModification("c.a", "1"))
      .then(() => testModification("Infinity", "Infinity"))
      .then(() => testModification("NaN", "NaN"))
      .then(() => testModification("new Function", "Function"))
      .then(() => testModification("+0", "0"))
      .then(() => testModification("-0", "-0"))
      .then(() => testModification("Object.keys({})", "Array"))
      .then(() => testModification("document.title", '"Debugger test page"'))
      .then(() => resumeDebuggerThenCloseAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });

    EventUtils.sendMouseEvent({ type: "click" },
      gDebuggee.document.querySelector("button"),
      gDebuggee);
  });
}

function initialChecks() {
  let localScope = gVars.getScopeAtIndex(0);
  let aVar = localScope.get("a");

  is(aVar.target.querySelector(".name").getAttribute("value"), "a",
    "Should have the right name for 'a'.");
  is(aVar.target.querySelector(".value").getAttribute("value"), "1",
    "Should have the right initial value for 'a'.");
}

function testModification(aNewValue, aNewResult) {
  let localScope = gVars.getScopeAtIndex(0);
  let aVar = localScope.get("a");

  
  
  executeSoon(() => {
    let varValue = aVar.target.querySelector(".title > .value");
    EventUtils.sendMouseEvent({ type: "mousedown" }, varValue, gDebugger);

    let varInput = aVar.target.querySelector(".title > .element-value-input");
    setText(varInput, aNewValue);
    EventUtils.sendKey("RETURN", gDebugger);
  });

  return waitForDebuggerEvents(gPanel, gDebugger.EVENTS.FETCHED_SCOPES).then(() => {
    let localScope = gVars.getScopeAtIndex(0);
    let aVar = localScope.get("a");

    is(aVar.target.querySelector(".name").getAttribute("value"), "a",
      "Should have the right name for 'a'.");
    is(aVar.target.querySelector(".value").getAttribute("value"), aNewResult,
      "Should have the right new value for 'a'.");
  });
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gVars = null;
});
