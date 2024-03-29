






const TAB_URL = EXAMPLE_URL + "doc_script-switching-01.html";

function test() {
  initDebugger(TAB_URL).then(([aTab,, aPanel]) => {
    let gTab = aTab;
    let gDebugger = aPanel.panelWin;
    let gEvents = gDebugger.EVENTS;
    let gEditor = gDebugger.DebuggerView.editor;
    let gSources = gDebugger.DebuggerView.Sources;
    let gBreakpoints = gDebugger.DebuggerController.Breakpoints;
    let gBreakpointLocation;
    Task.spawn(function*() {
      yield waitForSourceShown(aPanel, "-01.js");
      gBreakpointLocation = { actor: getSourceActor(gSources, EXAMPLE_URL + "code_script-switching-01.js"),
                              line: 5 };

      yield aPanel.addBreakpoint(gBreakpointLocation);

      yield ensureThreadClientState(aPanel, "resumed");
      yield testWhenBreakpointEnabledAndFirstSourceShown();

      yield reloadActiveTab(aPanel, gEvents.SOURCE_SHOWN);
      yield testWhenBreakpointEnabledAndSecondSourceShown();

      yield gSources.disableBreakpoint(gBreakpointLocation);
      yield reloadActiveTab(aPanel, gEvents.SOURCE_SHOWN);
      yield testWhenBreakpointDisabledAndSecondSourceShown();

      yield gSources.enableBreakpoint(gBreakpointLocation);
      yield reloadActiveTab(aPanel, gEvents.SOURCE_SHOWN);
      yield testWhenBreakpointEnabledAndSecondSourceShown();

      yield resumeDebuggerThenCloseAndFinish(aPanel);
    });

    function verifyView({ disabled, visible }) {
      return Task.spawn(function*() {
        
        
        yield waitForTick();

        let breakpointItem = gSources.getBreakpoint(gBreakpointLocation);
        let visibleBreakpoints = gEditor.getBreakpoints();
        is(!!breakpointItem.attachment.disabled, disabled,
          "The selected brekapoint state was correct.");
        is(breakpointItem.attachment.view.checkbox.hasAttribute("checked"), !disabled,
          "The selected brekapoint's checkbox state was correct.");

        
        let breakpointLine = breakpointItem.attachment.line - 1;
        let matchedBreakpoints = visibleBreakpoints.filter(e => e.line == breakpointLine);
        is(!!matchedBreakpoints.length, visible,
          "The selected breakpoint's visibility in the editor was correct.");
      });
    }

    
    

    function testWhenBreakpointEnabledAndFirstSourceShown() {
      return Task.spawn(function*() {
        yield ensureSourceIs(aPanel, "-01.js");
        yield verifyView({ disabled: false, visible: true });

        callInTab(gTab, "firstCall");
        yield waitForDebuggerEvents(aPanel, gEvents.FETCHED_SCOPES);
        yield ensureSourceIs(aPanel, "-01.js");
        yield ensureCaretAt(aPanel, 5);
        yield verifyView({ disabled: false, visible: true });

        executeSoon(() => gDebugger.gThreadClient.resume());
        yield waitForSourceAndCaretAndScopes(aPanel, "-02.js", 1);
        yield verifyView({ disabled: false, visible: false });
      });
    }

    function testWhenBreakpointEnabledAndSecondSourceShown() {
      return Task.spawn(function*() {
        yield ensureSourceIs(aPanel, "-02.js", true);
        yield verifyView({ disabled: false, visible: false });

        callInTab(gTab, "firstCall");
        yield waitForSourceAndCaretAndScopes(aPanel, "-01.js", 1);
        yield verifyView({ disabled: false, visible: true });

        executeSoon(() => gDebugger.gThreadClient.resume());
        yield waitForSourceAndCaretAndScopes(aPanel, "-02.js", 1);
        yield verifyView({ disabled: false, visible: false });
      });
    }

    function testWhenBreakpointDisabledAndSecondSourceShown() {
      return Task.spawn(function*() {
        yield ensureSourceIs(aPanel, "-02.js", true);
        yield verifyView({ disabled: true, visible: false });

        callInTab(gTab, "firstCall");
        yield waitForDebuggerEvents(aPanel, gEvents.FETCHED_SCOPES);
        yield ensureSourceIs(aPanel, "-02.js");
        yield ensureCaretAt(aPanel, 6);
        yield verifyView({ disabled: true, visible: false });

        executeSoon(() => gDebugger.gThreadClient.resume());
        yield waitForDebuggerEvents(aPanel, gEvents.AFTER_FRAMES_CLEARED);
        yield ensureSourceIs(aPanel, "-02.js");
        yield ensureCaretAt(aPanel, 6);
        yield verifyView({ disabled: true, visible: false });
      });
    }
  });
}
