





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, JsCallTreeView } = panel.panelWin;

  yield DetailsView.selectView("js-calltree");

  
  try {
    JsCallTreeView._onPrefChanged(null, "invert-call-tree", true);
    ok(true, "Toggling preferences before there are any recordings should not fail.");
  } catch (e) {
    ok(false, "Toggling preferences before there are any recordings should not fail.");
  }

  yield teardown(panel);
  finish();
}
