







const TAB_URL = EXAMPLE_URL + "doc_watch-expressions.html";

function test() {
  Task.spawn(function*() {
    let [tab,, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let vars = win.DebuggerView.Variables;

    win.DebuggerView.WatchExpressions.addExpression("this");

    callInTab(tab, "ermahgerd");
    yield waitForDebuggerEvents(panel, win.EVENTS.FETCHED_WATCH_EXPRESSIONS);

    let exprScope = vars.getScopeAtIndex(0);
    let {target} = exprScope.get("this");

    let name = target.querySelector(".title > .name");
    let separator = target.querySelector(".separator");
    let value = target.querySelector(".value");

    is(separator.hidden, false,
      "The separator element should not be hidden.");
    is(value.hidden, false,
      "The value element should not be hidden.");

    for (let key of ["ESCAPE", "RETURN"]) {
      EventUtils.sendMouseEvent({ type: "dblclick" }, name, win);

      is(separator.hidden, true,
        "The separator element should be hidden.");
      is(value.hidden, true,
        "The value element should be hidden.");

      EventUtils.sendKey(key, win);

      is(separator.hidden, false,
        "The separator element should not be hidden.");
      is(value.hidden, false,
        "The value element should not be hidden.");
    }

    yield resumeDebuggerThenCloseAndFinish(panel);
  });
}
