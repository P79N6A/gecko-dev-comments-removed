






const TAB_URL = EXAMPLE_URL + "doc_included-script.html";
const JS_URL = EXAMPLE_URL + "code_location-changes.js";

let gTab, gDebuggee, gPanel, gDebugger, gClient;
let gEditor, gSources, gControllerSources, gPrettyPrinted;

function test() {
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gClient = gDebugger.gClient;
    gEditor = gDebugger.DebuggerView.editor;
    gSources = gDebugger.DebuggerView.Sources;
    gControllerSources = gDebugger.DebuggerController.SourceScripts;

    
    
    
    
    gClient.request = (function (aOriginalRequestMethod) {
      return function (aPacket, aCallback) {
        if (aPacket.type == "prettyPrint") {
          gPrettyPrinted = true;
          return executeSoon(() => aCallback({ error: "prettyPrintError" }));
        }
        return aOriginalRequestMethod(aPacket, aCallback);
      };
    }(gClient.request));

    Task.spawn(function() {
      yield waitForSourceShown(gPanel, JS_URL);

      
      gEditor.once("change", () => {
        ok(false, "The source editor text shouldn't have changed.");
      });

      is(gSources.selectedValue, JS_URL,
        "The correct source is currently selected.");
      ok(gEditor.getText().contains("myFunction"),
        "The source shouldn't be pretty printed yet.");

      clickPrettyPrintButton();

      let { source } = gSources.selectedItem.attachment;
      try {
        yield gControllerSources.togglePrettyPrint(source);
        ok(false, "The promise for a prettified source should be rejected!");
      } catch ([source, error]) {
        ok(error.contains("prettyPrintError"),
          "The promise was correctly rejected with a meaningful message.");
      }

      let text;
      [source, text] = yield gControllerSources.getText(source);
      is(gSources.selectedValue, JS_URL,
        "The correct source is still selected.");
      ok(gEditor.getText().contains("myFunction"),
        "The displayed source hasn't changed.");
      ok(text.contains("myFunction"),
        "The cached source text wasn't altered in any way.");

      is(gPrettyPrinted, true,
        "The hijacked pretty print method was executed.");

      yield closeDebuggerAndFinish(gPanel);
    });
  });
}

function clickPrettyPrintButton() {
  gDebugger.document.getElementById("pretty-print").click();
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
  gClient = null;
  gEditor = null;
  gSources = null;
  gControllerSources = null;
  gPrettyPrinted = null;
});
