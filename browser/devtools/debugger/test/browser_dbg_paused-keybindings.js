





function test() {
  Task.spawn(function* () {
    const TAB_URL = EXAMPLE_URL + "doc_inline-script.html";
    let gDebugger, searchBox;

    let [, debuggee, panel] = yield initDebugger(TAB_URL);
    gDebugger = panel.panelWin;
    searchBox = gDebugger.DebuggerView.Filtering._searchbox;

    
    
    executeSoon(() => {
      EventUtils.sendMouseEvent({ type: "click" },
        debuggee.document.querySelector("button"),
        debuggee);
    });
    yield waitForSourceAndCaretAndScopes(panel, ".html", 20);
    yield ensureThreadClientState(panel, "paused");

    
    let tab2 = yield addTab(TAB_URL);
    yield removeTab(tab2);
    yield ensureCaretAt(panel, 20);

    
    let caretMove = ensureCaretAt(panel, 15, 1, true);
    
    executeSoon(function () {
      EventUtils.synthesizeKey("l", { accelKey: true });
      EventUtils.synthesizeKey("1", {});
      EventUtils.synthesizeKey("5", {});
    });
    yield caretMove;

    yield resumeDebuggerThenCloseAndFinish(panel);
  }).then(null, aError => {
    ok(false, "Got an error: " + aError.message + "\n" + aError.stack);
  });
}
