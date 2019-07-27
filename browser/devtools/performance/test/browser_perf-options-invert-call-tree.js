


const INVERT_PREF = "devtools.performance.ui.invert-call-tree";




function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, CallTreeView } = panel.panelWin;

  Services.prefs.setBoolPref(INVERT_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(CallTreeView, EVENTS.CALL_TREE_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  rendered = once(CallTreeView, EVENTS.CALL_TREE_RENDERED);
  Services.prefs.setBoolPref(INVERT_PREF, false);
  yield rendered;

  ok(true, "CallTreeView rerendered when toggling invert-call-tree.");

  rendered = once(CallTreeView, EVENTS.CALL_TREE_RENDERED);
  Services.prefs.setBoolPref(INVERT_PREF, true);
  yield rendered;

  ok(true, "CallTreeView rerendered when toggling back invert-call-tree.");

  yield teardown(panel);
  finish();
}
