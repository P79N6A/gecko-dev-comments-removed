







const TAB_URL = EXAMPLE_URL + "doc_recursion-stack.html";

function test() {
  Task.spawn(function() {
    let [tab, debuggee, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let events = win.EVENTS;
    let editor = win.DebuggerView.editor;
    let frames = win.DebuggerView.StackFrames;
    let variables = win.DebuggerView.Variables;
    let bubble = win.DebuggerView.VariableBubble;
    let tooltip = bubble._tooltip.panel;

    function checkView(selectedFrame, caretLine, debugLine = caretLine) {
      is(win.gThreadClient.state, "paused",
        "Should only be getting stack frames while paused.");
      is(frames.itemCount, 25,
        "Should have 25 frames.");
      is(frames.selectedDepth, selectedFrame,
        "The correct frame is selected in the widget.");
      ok(isCaretPos(panel, caretLine),
        "Editor caret location is correct.");
      ok(isDebugPos(panel, debugLine),
        "Editor caret location is correct.");
    }

    function expandGlobalScope() {
      let globalScope = variables.getScopeAtIndex(1);
      is(globalScope.expanded, false,
        "The globalScope should not be expanded yet.");

      let finished = waitForDebuggerEvents(panel, events.FETCHED_VARIABLES);
      globalScope.expand();
      return finished;
    }

    
    executeSoon(() => debuggee.recurse());
    yield waitForSourceAndCaretAndScopes(panel, ".html", 26);
    checkView(0, 26);

    yield expandGlobalScope();
    checkView(0, 26);

    
    yield openVarPopup(panel, { line: 26, ch: 11 });
    checkView(0, 26);

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
