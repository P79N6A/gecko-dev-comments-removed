







const TAB_URL = EXAMPLE_URL + "doc_breakpoints-other-tabs.html";

let test = Task.async(function* () {
  const [tab1, debuggee1, panel1] = yield initDebugger(TAB_URL);
  const [tab2, debuggee2, panel2] = yield initDebugger(TAB_URL);

  yield ensureSourceIs(panel1, "code_breakpoints-other-tabs.js", true);

  const sources = panel1.panelWin.DebuggerView.Sources;

  yield panel1.addBreakpoint({
    url: sources.selectedValue,
    line: 2
  });

  const paused = waitForThreadEvents(panel2, "paused");
  executeSoon(() => debuggee2.testCase());
  const packet = yield paused;

  is(packet.why.type, "debuggerStatement",
     "Should have stopped at the debugger statement, not the other tab's breakpoint");
  is(packet.frame.where.line, 3,
     "Should have stopped at line 3 (debugger statement), not line 2 (other tab's breakpoint)");

  yield teardown(panel1);
  yield resumeDebuggerThenCloseAndFinish(panel2);
});
