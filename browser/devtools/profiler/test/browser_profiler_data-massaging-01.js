s







const WAIT_TIME = 1000; 

let test = Task.async(function*() {
  let [target, debuggee, panel] = yield initFrontend(SIMPLE_URL);
  let front = panel.panelWin.gFront;

  

  yield front.startRecording();
  let profilingStartTime = front._profilingStartTime;
  info("Started profiling at: " + profilingStartTime);

  busyWait(WAIT_TIME); 

  let firstRecordingData = yield front.stopRecording();
  let firstRecordingFinishTime = firstRecordingData.profilerData.currentTime;

  is(profilingStartTime, 0,
    "The profiling start time should be 0 for the first recording.");
  ok(firstRecordingData.recordingDuration >= WAIT_TIME,
    "The first recording duration is correct.");
  ok(firstRecordingFinishTime >= WAIT_TIME,
    "The first recording finish time is correct.");

  

  yield front.startRecording();
  let profilingStartTime = front._profilingStartTime;
  info("Started profiling at: " + profilingStartTime);

  busyWait(WAIT_TIME); 

  let secondRecordingData = yield front.stopRecording();
  let secondRecordingFinishTime = secondRecordingData.profilerData.currentTime;
  let secondRecordingProfile = secondRecordingData.profilerData.profile;
  let secondRecordingSamples = secondRecordingProfile.threads[0].samples;

  isnot(profilingStartTime, 0,
    "The profiling start time should not be 0 on the second recording.");
  ok(secondRecordingData.recordingDuration >= WAIT_TIME,
    "The second recording duration is correct.");
  ok(secondRecordingFinishTime - firstRecordingFinishTime >= WAIT_TIME,
    "The second recording finish time is correct.");

  ok(secondRecordingSamples[0].time < profilingStartTime,
    "The second recorded sample times were normalized.");
  ok(secondRecordingSamples[0].time > 0,
    "The second recorded sample times were normalized correctly.");
  ok(!secondRecordingSamples.find(e => e.time + profilingStartTime <= firstRecordingFinishTime),
    "There should be no samples from the first recording in the second one, " +
    "even though the total number of frames did not overflow.");

  yield teardown(panel);
  finish();
});
