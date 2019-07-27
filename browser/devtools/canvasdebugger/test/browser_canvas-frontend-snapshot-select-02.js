







function ifTestingSupported() {
  let { target, panel } = yield initCanvasDebuggerFrontend(SIMPLE_CANVAS_URL);
  let { window, $, EVENTS, SnapshotsListView, CallsListView } = panel.panelWin;

  yield reload(target);

  SnapshotsListView._onRecordButtonClick();
  let snapshotTarget = SnapshotsListView.getItemAtIndex(0).target;

  EventUtils.sendMouseEvent({ type: "mousedown" }, snapshotTarget, window);
  EventUtils.sendMouseEvent({ type: "mousedown" }, snapshotTarget, window);
  EventUtils.sendMouseEvent({ type: "mousedown" }, snapshotTarget, window);

  ok(true, "clicking in-progress snapshot does not fail");

  let finished = once(window, EVENTS.SNAPSHOT_RECORDING_FINISHED);
  SnapshotsListView._onRecordButtonClick();
  yield finished;

  yield teardown(panel);
  finish();
}
