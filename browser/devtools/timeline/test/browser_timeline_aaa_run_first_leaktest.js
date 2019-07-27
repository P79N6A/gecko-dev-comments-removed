







let test = Task.async(function*() {
  let { target, panel } = yield initTimelinePanel(SIMPLE_URL);

  ok(target, "Should have a target available.");
  ok(panel, "Should have a panel available.");

  ok(panel.panelWin.gToolbox, "Should have a toolbox reference on the panel window.");
  ok(panel.panelWin.gTarget, "Should have a target reference on the panel window.");
  ok(panel.panelWin.gFront, "Should have a front reference on the panel window.");

  yield teardown(panel);
  finish();
});
