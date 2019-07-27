









function ifTestingSupported() {
  let { target, panel } = yield initCanvasDebuggerFrontend(RAF_BEGIN_URL);
  let { window, EVENTS, gFront, SnapshotsListView } = panel.panelWin;
  loadFrameScripts();

  yield reload(target);

  let recordingFinished = once(window, EVENTS.SNAPSHOT_RECORDING_FINISHED);
  SnapshotsListView._onRecordButtonClick();

  
  
  
  yield waitUntil(function*() { return !(yield gFront.isRecording()); });

  
  evalInDebuggee("start();");

  yield recordingFinished;
  ok(true, "Finished recording a snapshot of the animation loop.");

  yield removeTab(target.tab);
  finish();
}
