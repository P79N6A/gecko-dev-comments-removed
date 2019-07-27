





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, JsCallTreeView } = panel.panelWin;

  DetailsView.selectView("js-calltree");

  yield startRecording(panel);

  
  try {
    JsCallTreeView._onPrefChanged(null, "invert-call-tree", true);
    ok(true, "Toggling preferences during a recording should not fail.");
  } catch (e) {
    ok(false, "Toggling preferences during a recording should not fail.");
  }

  yield stopRecording(panel);

  yield teardown(panel);
  finish();
}
