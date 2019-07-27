







let test = Task.async(function*() {
  var { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  var { EVENTS, PerformanceController, WaterfallView } = panel.panelWin;

  yield startRecording(panel);
  yield stopRecording(panel);

  

  let file = FileUtils.getFile("TmpD", ["tmpprofile.json"]);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));

  let exported = once(PerformanceController, EVENTS.RECORDING_EXPORTED);
  yield PerformanceController.exportRecording("", PerformanceController.getCurrentRecording(), file);

  yield exported;
  ok(true, "The recording data appears to have been successfully saved.");

  

  yield PerformanceController.clearRecordings();

  let rendered = once(WaterfallView, EVENTS.WATERFALL_RENDERED);
  let imported = once(PerformanceController, EVENTS.RECORDING_IMPORTED);
  yield PerformanceController.importRecording("", file);
  yield imported;
  yield rendered;

  ok(true, "No error was thrown.");

  yield teardown(panel);
  finish();
});
