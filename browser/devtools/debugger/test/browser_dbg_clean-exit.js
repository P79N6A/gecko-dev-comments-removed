






let gTab, gDebuggee, gPanel, gDebugger;

const TAB_URL = EXAMPLE_URL + "doc_inline-debugger-statement.html";

function test() {
  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    gTab = aTab;
    gDebuggee = aDebuggee;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;

    testCleanExit();
  });
}

function testCleanExit() {
  waitForSourceAndCaretAndScopes(gPanel, ".html", 16).then(() => {
    is(gDebugger.gThreadClient.paused, true,
      "Should be paused after the debugger statement.");

    closeDebuggerAndFinish(gPanel, { whilePaused: true });
  });

  gDebuggee.runDebuggerStatement();
}

registerCleanupFunction(function() {
  gTab = null;
  gDebuggee = null;
  gPanel = null;
  gDebugger = null;
});
