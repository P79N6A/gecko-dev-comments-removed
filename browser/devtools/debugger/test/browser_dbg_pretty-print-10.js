







const TAB_URL = EXAMPLE_URL + "doc_pretty-print.html";

let gTab, gPanel, gDebugger;
let gEditor, gSources;

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gEditor = gDebugger.DebuggerView.editor;
    gSources = gDebugger.DebuggerView.Sources;

    waitForSourceShown(gPanel, "code_ugly.js")
      .then(testSourceIsUgly)
      .then(toggleBlackBoxing.bind(null, gPanel))
      .then(clickPrettyPrintButton)
      .then(testSourceIsStillUgly)
      .then(() => closeDebuggerAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + DevToolsUtils.safeErrorString(aError));
      });
  });
}

function testSourceIsUgly() {
  ok(!gEditor.getText().includes("\n    "),
     "The source shouldn't be pretty printed yet.");
}

function clickPrettyPrintButton() {
  
  
  return new Promise(resolve => {
    gDebugger.document.getElementById("pretty-print").click();
    resolve();
  });
}

function testSourceIsStillUgly() {
  const { source } = gSources.selectedItem.attachment;
  return gDebugger.DebuggerController.SourceScripts.getText(source).then(([, text]) => {
    ok(!text.includes("\n    "));
  });
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
  gEditor = null;
  gSources = null;
});
