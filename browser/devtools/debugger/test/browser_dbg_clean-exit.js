






let gTab, gPanel, gDebugger;

const TAB_URL = EXAMPLE_URL + "doc_inline-debugger-statement.html";

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;

    testCleanExit();
  });
}

function testCleanExit() {
  promise.all([
    waitForSourceAndCaretAndScopes(gPanel, ".html", 16),
    waitForDebuggerEvents(gPanel, gDebugger.EVENTS.AFTER_FRAMES_REFILLED)
  ]).then(() => {
    is(gDebugger.gThreadClient.paused, true,
      "Should be paused after the debugger statement.");
  }).then(() => closeDebuggerAndFinish(gPanel, { whilePaused: true }));

  callInTab(gTab, "runDebuggerStatement");
}

registerCleanupFunction(function() {
  gTab = null;
  gPanel = null;
  gDebugger = null;
});
