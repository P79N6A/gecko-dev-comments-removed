







function* spawnTest() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, gFront: front, PerformanceController } = panel.panelWin;

  let rec = yield front.startRecording({ withMarkers: true, withTicks: true, withMemory: true });
  ok(rec.isRecording(), "RecordingModel is recording when created");
  yield busyWait(100);
  yield waitUntil(() => rec.getMemory().length);
  ok(true, "RecordingModel populates memory while recording");
  yield waitUntil(() => rec.getTicks().length);
  ok(true, "RecordingModel populates ticks while recording");
  yield waitUntil(() => rec.getMarkers().length);
  ok(true, "RecordingModel populates markers while recording");

  ok(!rec.isCompleted(), "RecordingModel is not completed when still recording");

  let stopping = once(front, "recording-stopping");
  let stopped = once(front, "recording-stopped");
  front.stopRecording(rec);

  yield stopping;
  ok(!rec.isRecording(), "on 'recording-stopping', model is no longer recording");
  
  
  
  if (rec.getProfile()) {
    ok(rec.isCompleted(), "recording is completed once it has profile data");
  } else {
    ok(!rec.isCompleted(), "recording is not yet completed on 'recording-stopping'");
  }

  yield stopped;
  ok(!rec.isRecording(), "on 'recording-stopped', model is still no longer recording");
  ok(rec.isCompleted(), "on 'recording-stopped', model is considered 'complete'");

  
  let file = FileUtils.getFile("TmpD", ["tmpprofile.json"]);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));
  let exported = once(PerformanceController, EVENTS.RECORDING_EXPORTED);
  yield PerformanceController.exportRecording("", rec, file);
  yield exported;

  let imported = once(PerformanceController, EVENTS.RECORDING_IMPORTED);
  yield PerformanceController.importRecording("", file);

  yield imported;
  let importedModel = PerformanceController.getCurrentRecording();

  ok(importedModel.isCompleted(), "All imported recordings should be completed");
  ok(!importedModel.isRecording(), "All imported recordings should not be recording");

  yield teardown(panel);
  finish();
}
