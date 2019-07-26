








const TAB_URL = EXAMPLE_URL + "doc_auto-pretty-print-02.html";

let gTab, gDebuggee, gPanel, gDebugger;
let gEditor, gSources, gPrefs, gOptions, gView;

let gFirstSourceLabel = "code_ugly-6.js";
let gSecondSourceLabel = "code_ugly-7.js";

function test(){
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gEditor = gDebugger.DebuggerView.editor;
    gSources = gDebugger.DebuggerView.Sources;
    gPrefs = gDebugger.Prefs;
    gOptions = gDebugger.DebuggerView.Options;
    gView = gDebugger.DebuggerView;

    
    testAutoPrettyPrintOn();

    waitForSourceShown(gPanel, gFirstSourceLabel)
      .then(testSourceIsUgly)
      .then(() => waitForSourceShown(gPanel, gFirstSourceLabel))
      .then(testSourceIsPretty)
      .then(testPrettyPrintButtonOn)
      .then(() => {
        
        let finished = waitForDebuggerEvents(gPanel, gDebugger.EVENTS.SOURCE_SHOWN);
        gSources.selectedIndex = 1;
        return finished;
      })
      .then(testSecondSourceLabel)
      .then(() => {
        
        let finished = waitForDebuggerEvents(gPanel, gDebugger.EVENTS.SOURCE_SHOWN);
        gSources.selectedIndex = 0;
        return finished;
      })
      .then(testFirstSourceLabel)
      .then(testPrettyPrintButtonOn)
      
      .then(disableAutoPrettyPrint)
      .then(() => closeDebuggerAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + DevToolsUtils.safeErrorString(aError));
      })
  });
}

function testSourceIsUgly() {
  ok(!gEditor.getText().contains("\n    "),
    "The source shouldn't be pretty printed yet.");
}

function testFirstSourceLabel(){
  ok(gSources.containsValue(EXAMPLE_URL + gFirstSourceLabel),
    "First source url is correct.");
}

function testSecondSourceLabel(){
  ok(gSources.containsValue(EXAMPLE_URL + gSecondSourceLabel),
    "Second source url is correct.");
}

function testAutoPrettyPrintOn(){
  is(gPrefs.autoPrettyPrint, true,
    "The auto-pretty-print pref should be on.");
  is(gOptions._autoPrettyPrint.getAttribute("checked"), "true",
    "The Auto pretty print menu item should be checked.");
}

function testPrettyPrintButtonOn(){
  is(gDebugger.document.getElementById("pretty-print").checked, true,
    "The button should be checked when the source is selected.");
}

function disableAutoPrettyPrint(){
  gOptions._autoPrettyPrint.setAttribute("checked", "false");
  gOptions._toggleAutoPrettyPrint();
  gOptions._onPopupHidden();
}

function testSourceIsPretty() {
  ok(gEditor.getText().contains("\n    "),
    "The source should be pretty printed.")
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gEditor = null;
  gSources = null;
  gOptions = null;
  gPrefs = null;
  gView = null;
});
