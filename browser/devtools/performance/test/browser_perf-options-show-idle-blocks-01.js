





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, JsFlameGraphView } = panel.panelWin;

  Services.prefs.setBoolPref(IDLE_PREF, true);

  yield startRecording(panel);
  yield busyWait(100);
  yield stopRecording(panel);

  let rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  yield DetailsView.selectView("js-flamegraph");
  yield rendered;

  rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(IDLE_PREF, false);
  yield rendered;

  ok(true, "JsFlameGraphView rerendered when toggling show-idle-blocks.");

  rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(IDLE_PREF, true);
  yield rendered;

  ok(true, "JsFlameGraphView rerendered when toggling back show-idle-blocks.");

  yield teardown(panel);
  finish();
}
