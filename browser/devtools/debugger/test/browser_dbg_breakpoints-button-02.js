







const TAB_URL = EXAMPLE_URL + "doc_script-switching-01.html";

function test() {
  let gTab, gPanel, gDebugger;
  let gSources, gBreakpoints;

  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    gTab = aTab;
    gPanel = aPanel;
    gDebugger = gPanel.panelWin;
    gSources = gDebugger.DebuggerView.Sources;
    gBreakpoints = gDebugger.DebuggerController.Breakpoints;

    waitForSourceShown(gPanel, "-01.js")
      .then(addBreakpoints)
      .then(disableSomeBreakpoints)
      .then(testToggleBreakpoints)
      .then(testEnableBreakpoints)
      .then(() => ensureThreadClientState(gPanel, "resumed"))
      .then(() => closeDebuggerAndFinish(gPanel))
      .then(null, aError => {
        ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
      });
  });

  function addBreakpoints() {
    return promise.resolve(null)
      .then(() => gPanel.addBreakpoint({ actor: gSources.values[0], line: 5 }))
      .then(() => gPanel.addBreakpoint({ actor: gSources.values[1], line: 6 }))
      .then(() => gPanel.addBreakpoint({ actor: gSources.values[1], line: 7 }))
      .then(() => ensureThreadClientState(gPanel, "resumed"));
  }

  function disableSomeBreakpoints() {
    return promise.all([
      gSources.disableBreakpoint({ actor: gSources.values[0], line: 5 }),
      gSources.disableBreakpoint({ actor: gSources.values[1], line: 6 })
    ]);
  }

  function testToggleBreakpoints() {
    let finished = waitForDebuggerEvents(gPanel, gDebugger.EVENTS.BREAKPOINT_REMOVED, 1);
    gSources.toggleBreakpoints();
    return finished.then(() => checkBreakpointsDisabled(true));
  }

  function testEnableBreakpoints() {
    let finished = waitForDebuggerEvents(gPanel, gDebugger.EVENTS.BREAKPOINT_ADDED, 3);
    gSources.toggleBreakpoints();
    return finished.then(() => checkBreakpointsDisabled(false));
  }

  function checkBreakpointsDisabled(aState, aTotal = 3) {
    let breakpoints = gSources.getAllBreakpoints();

    is(breakpoints.length, aTotal,
      "Breakpoints should still be set.");
    is(breakpoints.filter(e => e.attachment.disabled == aState).length, aTotal,
      "Breakpoints should be " + (aState ? "disabled" : "enabled") + ".");
  }
}
