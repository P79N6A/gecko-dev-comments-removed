






const TAB_URL = EXAMPLE_URL + "doc_frame-parameters.html";

function test() {
  Task.spawn(function() {
    let [tab, debuggee, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let bubble = win.DebuggerView.VariableBubble;
    let tooltip = bubble._tooltip.panel;

    
    executeSoon(() => debuggee.start());
    yield waitForSourceAndCaretAndScopes(panel, ".html", 24);

    yield openVarPopup(panel, { line: 15, ch: 12 });
    ok(true, "The variable inspection popup was shown for the real variable.");

    once(tooltip, "popupshown").then(() => {
      ok(false, "The variable inspection popup shouldn't have been opened.");
    });

    reopenVarPopup(panel, { line: 17, ch: 27 });
    yield waitForTime(1000);

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
