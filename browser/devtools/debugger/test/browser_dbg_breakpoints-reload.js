







const TAB_URL = EXAMPLE_URL + "doc_breakpoints-reload.html";

let test = Task.async(function* () {
  requestLongerTimeout(4);

  const [tab,, panel] = yield initDebugger(TAB_URL);

  yield ensureSourceIs(panel, "doc_breakpoints-reload.html", true);

  const sources = panel.panelWin.DebuggerView.Sources;

  yield panel.addBreakpoint({
    url: sources.selectedValue,
    line: 10 
  });

  const paused = waitForThreadEvents(panel, "paused");
  reloadActiveTab(panel);
  const packet = yield paused;

  is(packet.why.type, "breakpoint",
     "Should have hit the breakpoint after the reload");
  is(packet.frame.where.line, 10,
     "Should have stopped at line 10, where we set the breakpoint");

  yield resumeDebuggerThenCloseAndFinish(panel);
});
