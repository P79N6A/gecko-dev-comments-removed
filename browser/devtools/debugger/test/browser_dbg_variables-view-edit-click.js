







const TAB_URL = EXAMPLE_URL + "doc_watch-expressions.html";

function test() {
  Task.spawn(function*() {
    let [tab, debuggee, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let vars = win.DebuggerView.Variables;

    win.DebuggerView.WatchExpressions.addExpression("this");

    
    executeSoon(() => debuggee.ermahgerd());
    yield waitForDebuggerEvents(panel, win.EVENTS.FETCHED_WATCH_EXPRESSIONS);

    let exprScope = vars.getScopeAtIndex(0);
    let exprVar = exprScope.get("this");
    let name = exprVar.target.querySelector(".title > .name");

    is(exprVar.editing, false,
      "The expression should indicate it is not being edited.");

    EventUtils.sendMouseEvent({ type: "dblclick" }, name, win);
    let input = exprVar.target.querySelector(".title > .element-name-input");
    is(exprVar.editing, true,
      "The expression should indicate it is being edited.");
    is(input.selectionStart !== input.selectionEnd, true,
      "The expression text should be selected.");

    EventUtils.synthesizeMouse(input, 2, 2, {}, win);
    is(exprVar.editing, true,
      "The expression should indicate it is still being edited after a click.");
    is(input.selectionStart === input.selectionEnd, true,
      "The expression text should not be selected.");

    EventUtils.sendKey("ESCAPE", win);
    is(exprVar.editing, false,
      "The expression should indicate it is not being edited after cancelling.");

    
    EventUtils.synthesizeMouse(vars.parentNode, 2, 2, {}, win);

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
