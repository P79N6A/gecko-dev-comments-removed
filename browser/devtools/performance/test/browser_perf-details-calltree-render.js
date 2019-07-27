





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView, CallTreeView } = panel.panelWin;

  DetailsView.selectView("calltree");
  ok(DetailsView.isViewSelected(CallTreeView), "The call tree is now selected.");

  yield startRecording(panel);
  yield busyWait(100);

  let rendered = once(CallTreeView, EVENTS.CALL_TREE_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "CallTreeView rendered after recording is stopped.");

  yield startRecording(panel);
  yield busyWait(100);

  rendered = once(CallTreeView, EVENTS.CALL_TREE_RENDERED);
  yield stopRecording(panel);
  yield rendered;

  ok(true, "CallTreeView rendered again after recording completed a second time.");

  yield teardown(panel);
  finish();
}
