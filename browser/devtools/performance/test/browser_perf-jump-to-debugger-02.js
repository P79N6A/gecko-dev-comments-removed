







function spawnTest() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL, "jsdebugger");
  let debuggerWin = panel.panelWin;
  let debuggerEvents = debuggerWin.EVENTS;
  let { DebuggerView } = debuggerWin;
  let Sources = DebuggerView.Sources;

  yield debuggerWin.once(debuggerEvents.SOURCE_SHOWN);
  ok("A source was shown in the debugger.");

  is(Sources.selectedValue, getSourceActor(Sources, SIMPLE_URL),
    "The correct source is initially shown in the debugger.");
  is(DebuggerView.editor.getCursor().line, 0,
    "The correct line is initially highlighted in the debugger's source editor.");

  yield toolbox.selectTool("performance");
  let perfPanel = toolbox.getCurrentPanel();
  let perfWin = perfPanel.panelWin;
  let { viewSourceInDebugger } = perfWin;

  yield viewSourceInDebugger(SIMPLE_URL, 14);

  panel = toolbox.getPanel("jsdebugger");
  ok(panel, "The debugger panel was reselected.");

  is(DebuggerView.Sources.selectedValue, getSourceActor(Sources, SIMPLE_URL),
    "The correct source is still shown in the debugger.");
  is(DebuggerView.editor.getCursor().line + 1, 14,
    "The correct line is now highlighted in the debugger's source editor.");

  yield teardown(perfPanel);
  finish();
}
