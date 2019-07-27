






let test = Task.async(function*() {
  var { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  var { EVENTS, PerformanceController, DetailsView, DetailsSubview } = panel.panelWin;

  
  Services.prefs.setBoolPref(ALLOCATIONS_PREF, true);
  Services.prefs.setBoolPref(MEMORY_PREF, true);
  Services.prefs.setBoolPref(FRAMERATE_PREF, true);

  
  
  DetailsSubview.canUpdateWhileHidden = true;

  yield startRecording(panel);
  yield stopRecording(panel);

  
  
  
  yield DetailsView.selectView("js-calltree");
  yield DetailsView.selectView("js-flamegraph");
  yield DetailsView.selectView("memory-calltree");
  yield DetailsView.selectView("memory-flamegraph");


  

  let originalData = PerformanceController.getCurrentRecording().getAllData();
  ok(originalData, "The original recording is not empty.");

  

  let file = FileUtils.getFile("TmpD", ["tmpprofile.json"]);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));

  let exported = once(PerformanceController, EVENTS.RECORDING_EXPORTED);
  yield PerformanceController.exportRecording("", PerformanceController.getCurrentRecording(), file);

  yield exported;
  ok(true, "The recording data appears to have been successfully saved.");

  

  let rerendered = waitForWidgetsRendered(panel);
  let imported = once(PerformanceController, EVENTS.RECORDING_IMPORTED);
  yield PerformanceController.importRecording("", file);

  yield imported;
  ok(true, "The recording data appears to have been successfully imported.");

  yield rerendered;
  ok(true, "The imported data was re-rendered.");

  

  let importedData = PerformanceController.getCurrentRecording().getAllData();

  is(importedData.label, originalData.label,
    "The imported data is identical to the original data (1).");
  is(importedData.duration, originalData.duration,
    "The imported data is identical to the original data (2).");
  is(importedData.markers.toSource(), originalData.markers.toSource(),
    "The imported data is identical to the original data (3).");
  is(importedData.memory.toSource(), originalData.memory.toSource(),
    "The imported data is identical to the original data (4).");
  is(importedData.ticks.toSource(), originalData.ticks.toSource(),
    "The imported data is identical to the original data (5).");
  is(importedData.allocations.toSource(), originalData.allocations.toSource(),
    "The imported data is identical to the original data (6).");
  is(importedData.profile.toSource(), originalData.profile.toSource(),
    "The imported data is identical to the original data (7).");
  is(importedData.configuration.withTicks, originalData.configuration.withTicks,
    "The imported data is identical to the original data (8).");
  is(importedData.configuration.withMemory, originalData.configuration.withMemory,
    "The imported data is identical to the original data (9).");

  yield teardown(panel);

  
  
  
  var { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  var { EVENTS, PerformanceController, DetailsView, DetailsSubview, OverviewView, WaterfallView } = panel.panelWin;
  yield PerformanceController.clearRecordings();

  rerendered = once(WaterfallView, EVENTS.WATERFALL_RENDERED);
  imported = once(PerformanceController, EVENTS.RECORDING_IMPORTED);
  yield PerformanceController.importRecording("", file);

  yield imported;
  ok(true, "The recording data appears to have been successfully imported.");

  yield rerendered;
  ok(true, "The imported data was re-rendered.");

  yield teardown(panel);
  finish();
});
