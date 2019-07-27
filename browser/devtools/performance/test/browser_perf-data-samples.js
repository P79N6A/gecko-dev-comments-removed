








const WAIT_TIME = 1000; 

function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let front = panel.panelWin.gFront;

  yield front.startRecording();
  busyWait(WAIT_TIME); 

  let recordingData = yield front.stopRecording();
  let profile = recordingData.profilerData.profile;

  for (let thread of profile.threads) {
    info("Checking thread: " + thread.name);

    for (let sample of thread.samples) {
      if (sample.frames[0].location != "(root)") {
        ok(false, "The sample " + sample.toSource() + " doesn't have a root node.");
      }
    }
  }

  yield teardown(panel);
  finish();
}
