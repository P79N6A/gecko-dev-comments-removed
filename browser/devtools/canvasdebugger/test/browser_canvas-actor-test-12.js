







function ifTestingSupported() {
  let { target, front } = yield initCanvasDebuggerBackend(NO_CANVAS_URL);
  loadFrameScripts();

  let navigated = once(target, "navigate");

  yield front.setup({ reload: true });
  ok(true, "The front was setup up successfully.");

  yield navigated;
  ok(true, "Target automatically navigated when the front was set up.");

  let startRecording = front.recordAnimationFrame();
  yield front.stopRecordingAnimationFrame();

  ok(!(yield startRecording),
    "recordAnimationFrame() does not return a SnapshotActor when cancelled.");

  yield removeTab(target.tab);
  finish();
}
