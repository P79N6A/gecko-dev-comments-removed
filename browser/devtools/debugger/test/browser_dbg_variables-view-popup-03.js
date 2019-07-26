






const TAB_URL = EXAMPLE_URL + "doc_frame-parameters.html";

function test() {
  Task.spawn(function() {
    let [tab, debuggee, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let bubble = win.DebuggerView.VariableBubble;

    
    executeSoon(() => debuggee.start());
    yield waitForSourceAndCaretAndScopes(panel, ".html", 24);

    
    yield openVarPopup(panel, { line: 15, ch: 12 });

    ok(!bubble._tooltip.isEmpty(),
      "The variable inspection popup isn't empty.");
    ok(bubble._markedText,
      "There's some marked text in the editor.");
    ok(bubble._markedText.clear,
      "The marked text in the editor can be cleared.");

    yield hideVarPopup(panel);

    ok(bubble._tooltip.isEmpty(),
      "The variable inspection popup is now empty.");
    ok(!bubble._markedText,
      "The marked text in the editor was removed.");

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
