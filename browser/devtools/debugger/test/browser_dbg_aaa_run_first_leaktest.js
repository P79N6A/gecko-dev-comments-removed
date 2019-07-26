








const TAB_URL = EXAMPLE_URL + "doc_script-switching-01.html";

function test() {
  
  
  requestLongerTimeout(2);

  initDebugger(TAB_URL).then(([aTab, aDebuggee, aPanel]) => {
    ok(aTab, "Should have a tab available.");
    ok(aDebuggee, "Should have a debuggee available.");
    ok(aPanel, "Should have a debugger pane available.");

    waitForSourceAndCaretAndScopes(aPanel, "-02.js", 6).then(() => {
      resumeDebuggerThenCloseAndFinish(aPanel);
    });

    aDebuggee.firstCall();
  });
}
