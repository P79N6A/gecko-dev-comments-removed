







function test() {
  Task.spawn(runTests);
}

function* runTests() {
  let TAB_URL = EXAMPLE_URL + "doc_empty-tab-01.html";
  let [,, panel] = yield initDebugger(TAB_URL);
  let dbgWin = panel.panelWin;
  let sources = dbgWin.DebuggerView.Sources;
  let frames = dbgWin.DebuggerView.StackFrames;
  let editor = dbgWin.DebuggerView.editor;
  let toolbox = gDevTools.getToolbox(panel.target);

  let paused = waitForSourceAndCaretAndScopes(panel, "foo.js", 1);

  toolbox.once("webconsole-ready", () => {
    ok(toolbox.splitConsole, "Split console is shown.");
    let jsterm = toolbox.getPanel("webconsole").hud.jsterm;
    jsterm.execute("eval('debugger; //# sourceURL=foo.js')");
  });
  EventUtils.synthesizeKey("VK_ESCAPE", {}, dbgWin);

  yield paused;
  is(sources.selectedItem.attachment.label, 'foo.js',
     'New source is selected in sources');
  is(sources.selectedItem.attachment.group, 'http://example.com',
     'New source is in the right group');
  ok(editor.getText() === 'debugger; //# sourceURL=foo.js', 'Editor has correct text');

  yield toolbox.closeSplitConsole();
  yield resumeDebuggerThenCloseAndFinish(panel);
}
