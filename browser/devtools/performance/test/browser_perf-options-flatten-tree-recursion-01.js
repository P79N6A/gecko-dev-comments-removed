





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, JsFlameGraphView } = panel.panelWin;

  DetailsView.selectView("js-flamegraph");

  Services.prefs.setBoolPref(FLATTEN_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(FLATTEN_PREF, false);
  yield rendered;

  ok(true, "JsFlameGraphView rerendered when toggling flatten-tree-recursion.");

  rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(FLATTEN_PREF, true);
  yield rendered;

  ok(true, "JsFlameGraphView rerendered when toggling back flatten-tree-recursion.");

  yield teardown(panel);
  finish();
}
