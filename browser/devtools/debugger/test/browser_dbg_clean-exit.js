






var gPane = null;
var gTab = null;
var gDebugger = null;

const DEBUGGER_TAB_URL = EXAMPLE_URL + "browser_dbg_debuggerstatement.html";

function test() {
  debug_tab_pane(DEBUGGER_TAB_URL, function(aTab, aDebuggee, aPane) {
    gTab = aTab;
    gPane = aPane;
    gDebugger = gPane.panelWin;

    testCleanExit();
  });
}

function testCleanExit() {
  gDebugger.DebuggerController.activeThread.addOneTimeListener("framesadded", function() {
    Services.tm.currentThread.dispatch({ run: function() {

      is(gDebugger.DebuggerController.activeThread.paused, true,
        "Should be paused after the debugger statement.");

      closeDebuggerAndFinish();
    }}, 0);
  });

  gTab.linkedBrowser.contentWindow.wrappedJSObject.runDebuggerStatement();
}

registerCleanupFunction(function() {
  removeTab(gTab);
  gPane = null;
  gTab = null;
  gDebugger = null;
});
