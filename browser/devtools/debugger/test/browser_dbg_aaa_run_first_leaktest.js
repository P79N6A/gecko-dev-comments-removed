








const TAB_URL = EXAMPLE_URL + "doc_script-switching-01.html";

function test() {
  
  
  requestLongerTimeout(2);

  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    ok(aTab, "Should have a tab available.");
    ok(aPanel, "Should have a debugger pane available.");

    waitForSourceAndCaretAndScopes(aPanel, "-02.js", 1).then(() => {
      resumeDebuggerThenCloseAndFinish(aPanel);
    });

    callInTab(aTab, "firstCall");
  });
}
