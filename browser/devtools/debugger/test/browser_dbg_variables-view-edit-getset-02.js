







function test() {
  const TAB_URL = EXAMPLE_URL + "doc_frame-parameters.html";

  
  requestLongerTimeout(2);

  Task.spawn(function* () {
    let [, debuggee, panel] = yield initDebugger(TAB_URL);
    let win = panel.panelWin;
    let L10N = win.L10N;
    let editor = win.DebuggerView.editor;
    let vars = win.DebuggerView.Variables;
    let watch = win.DebuggerView.WatchExpressions;

    vars.switch = function() {};
    vars.delete = function() {};

    let paused = waitForSourceAndCaretAndScopes(panel, ".html", 24);
    
    
    executeSoon(() => {
      EventUtils.sendMouseEvent({ type: "click" },
        debuggee.document.querySelector("button"),
        debuggee);
    });
    yield paused;

    
    let addedWatch = waitForDebuggerEvents(panel, win.EVENTS.FETCHED_WATCH_EXPRESSIONS);
    watch.addExpression("myVar.prop");
    editor.focus();
    yield addedWatch;

    
    vars.focusLastVisibleItem();

    let localScope = vars.getScopeAtIndex(1);
    let myVar = localScope.get("myVar");

    myVar.expand();
    vars.clearHierarchy();

    yield waitForDebuggerEvents(panel, win.EVENTS.FETCHED_PROPERTIES);

    let editTarget = myVar.get("prop").target;

    
    
    executeSoon(() => {
      let varEdit = editTarget.querySelector(".title > .variables-view-edit");
      EventUtils.sendMouseEvent({ type: "mousedown" }, varEdit, win);

      let varInput = editTarget.querySelector(".title > .element-value-input");
      setText(varInput, "\"xlerb\"");
      EventUtils.sendKey("RETURN", win);
    });

    yield waitForDebuggerEvents(panel, win.EVENTS.FETCHED_WATCH_EXPRESSIONS);

    let exprScope = vars.getScopeAtIndex(0);

    ok(exprScope,
      "There should be a wach expressions scope in the variables view.");
    is(exprScope.name, L10N.getStr("watchExpressionsScopeLabel"),
      "The scope's name should be marked as 'Watch Expressions'.");
    is(exprScope._store.size, 1,
      "There should be one evaluation available.");

    is(exprScope.get("myVar.prop").value, "xlerb",
      "The expression value is correct after the edit.");

    yield resumeDebuggerThenCloseAndFinish(panel);
  }).then(null, aError => {
      ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
  });
}
