





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, JsCallTreeView } = panel.panelWin;

  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);
  yield stopRecording(panel);

  let rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
  yield DetailsView.selectView("js-calltree");
  yield rendered;

  rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, false);
  yield rendered;
  ok(true, "JsCallTreeView rerendered when toggling off show-platform-data.");

  rendered = once(JsCallTreeView, EVENTS.JS_CALL_TREE_RENDERED);
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, true);
  yield rendered;
  ok(true, "JsCallTreeView rerendered when toggling on show-platform-data.");

  yield teardown(panel);
  finish();
}
