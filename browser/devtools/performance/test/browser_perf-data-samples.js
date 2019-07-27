








const WAIT_TIME = 1000; 

function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let front = panel.panelWin.gFront;

  let rec = yield front.startRecording();
  busyWait(WAIT_TIME); 

  yield front.stopRecording(rec);
  let profile = rec.getProfile();
  let sampleCount = 0;

  for (let thread of profile.threads) {
    info("Checking thread: " + thread.name);

    for (let sample of thread.samples) {
      sampleCount++;

      if (sample.frames[0].location != "(root)") {
        ok(false, "The sample " + sample.toSource() + " doesn't have a root node.");
      }
    }
  }

  ok(sampleCount > 0,
    "At least some samples have been iterated over, checking for root nodes.");

  yield teardown(panel);
  finish();
}
