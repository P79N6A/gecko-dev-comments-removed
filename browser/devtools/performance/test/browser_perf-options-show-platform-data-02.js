





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, JsFlameGraphView } = panel.panelWin;

  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, false);

  yield startRecording(panel);
  yield busyWait(100);
  yield stopRecording(panel);

  let rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  yield DetailsView.selectView("js-flamegraph");
  yield rendered;

  rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, true);
  yield rendered;
  ok(true, "JsFlameGraphView rerendered when toggling on show-platform-data.");

  rendered = once(JsFlameGraphView, EVENTS.JS_FLAMEGRAPH_RENDERED);
  Services.prefs.setBoolPref(PLATFORM_DATA_PREF, false);
  yield rendered;
  ok(true, "JsFlameGraphView rerendered when toggling off show-platform-data.");

  yield teardown(panel);
  finish();
}
