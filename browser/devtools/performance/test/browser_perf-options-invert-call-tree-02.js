






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, MemoryCallTreeView } = panel.panelWin;

  
  Services.prefs.setBoolPref(MEMORY_PREF, true);
  Services.prefs.setBoolPref(INVERT_PREF, true);

  DetailsView.selectView("memory-calltree");
  ok(DetailsView.isViewSelected(MemoryCallTreeView), "The call tree is now selected.");

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(MemoryCallTreeView, EVENTS.MEMORY_CALL_TREE_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  rendered = once(MemoryCallTreeView, EVENTS.MEMORY_CALL_TREE_RENDERED);
  Services.prefs.setBoolPref(INVERT_PREF, false);
  yield rendered;

  ok(true, "MemoryCallTreeView rerendered when toggling invert-call-tree.");

  rendered = once(MemoryCallTreeView, EVENTS.MEMORY_CALL_TREE_RENDERED);
  Services.prefs.setBoolPref(INVERT_PREF, true);
  yield rendered;

  ok(true, "MemoryCallTreeView rerendered when toggling back invert-call-tree.");

  yield teardown(panel);
  finish();
}
