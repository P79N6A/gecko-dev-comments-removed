








const TAB_URL = EXAMPLE_URL + "doc_global-method-override.html";

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    let gDebugger = aPanel.panelWin;
    ok(gDebugger, "Should have a debugger available.");
    is(gDebugger.gThreadClient.state, "attached", "Debugger should be attached.");

    closeDebuggerAndFinish(aPanel);
  });
}
