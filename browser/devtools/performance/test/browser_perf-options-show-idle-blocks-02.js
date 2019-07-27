





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, MemoryFlameGraphView } = panel.panelWin;

  
  Services.prefs.setBoolPref(MEMORY_PREF, true);
  DetailsView.selectView("memory-flamegraph");

  Services.prefs.setBoolPref(IDLE_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  rendered = once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(IDLE_PREF, false);
  yield rendered;

  ok(true, "MemoryFlameGraphView rerendered when toggling show-idle-blocks.");

  rendered = once(MemoryFlameGraphView, EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(IDLE_PREF, true);
  yield rendered;

  ok(true, "MemoryFlameGraphView rerendered when toggling back show-idle-blocks.");

  yield teardown(panel);
  finish();
}
