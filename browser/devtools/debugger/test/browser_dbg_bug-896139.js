






const TAB_URL = "doc_bug-896139.html";
const SCRIPT_URL = "code_bug-896139.js";

function test() {
  Task.spawn(function* () {
    function testBreakpoint() {
      let promise = waitForDebuggerEvents(panel, win.EVENTS.FETCHED_SCOPES);
      callInTab(tab, "f");
      return promise.then(() => doResume(panel));
    }

    let [tab,, panel] = yield initDebugger(EXAMPLE_URL + TAB_URL);
    let win = panel.panelWin;
    yield waitForSourceShown(panel, SCRIPT_URL);
    yield panel.addBreakpoint({
      actor: getSourceActor(win.DebuggerView.Sources, EXAMPLE_URL + SCRIPT_URL),
      line: 6
    });

    
    
    
    if(panel.panelWin.gThreadClient.state !== "attached") {
      yield waitForThreadEvents(panel, "resumed");
    }

    yield testBreakpoint();
    yield reloadActiveTab(panel, win.EVENTS.SOURCE_SHOWN);
    yield testBreakpoint();

    yield closeDebuggerAndFinish(panel);
  });
}
