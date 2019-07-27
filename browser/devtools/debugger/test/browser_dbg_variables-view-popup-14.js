







const TAB_URL = EXAMPLE_URL + "doc_frame-parameters.html";

function test() {
  Task.spawn(function*() {
    let [tab,, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let bubble = win.DebuggerView.VariableBubble;

    callInTab(tab, "start");
    yield waitForSourceAndCaretAndScopes(panel, ".html", 24);

    
    let cursor = win.DebuggerView.editor.getOffset({ line: 15, ch: 12 });
    let [ anchor, head ] = win.DebuggerView.editor.getPosition(
      cursor,
      cursor + 3
    );
    win.DebuggerView.editor.setSelection(anchor, head);

    
    let popupOpened = yield intendOpenVarPopup(panel, { line: 15, ch: 12 }, true);

    
    ok(!popupOpened,
      "The popup is not opened");
    ok(!bubble._markedText,
      "The marked text in the editor is not there.");

    
    popupOpened = yield intendOpenVarPopup(panel, { line: 15, ch: 12 }, false);

    
    ok(popupOpened,
      "The popup is opened");
    ok(bubble._markedText,
      "The marked text in the editor is there.");

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
