





function spawnTest () {
  
  Services.prefs.setBoolPref(MEMORY_PREF, true);

  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, MemoryFlameGraphView } = panel.panelWin;

  DetailsView.selectView("memory-flamegraph");

  Services.prefs.setBoolPref(FLATTEN_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  rendered = once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(FLATTEN_PREF, false);
  yield rendered;

  ok(true, "MemoryFlameGraphView rerendered when toggling flatten-tree-recursion.");

  rendered = once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(FLATTEN_PREF, true);
  yield rendered;

  ok(true, "MemoryFlameGraphView rerendered when toggling back flatten-tree-recursion.");

  yield teardown(panel);
  finish();
}
