







let test = Task.async(function*() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, DetailsView, DetailsSubview } = panel.panelWin;

  
  Services.prefs.setBoolPref(MEMORY_PREF, true);

  
  
  
  yield DetailsView.selectView("js-calltree");
  yield DetailsView.selectView("js-flamegraph");
  yield DetailsView.selectView("memory-calltree");
  yield DetailsView.selectView("memory-flamegraph");

  
  
  DetailsSubview.canUpdateWhileHidden = true;

  yield startRecording(panel);
  yield stopRecording(panel);

  
  let data = PerformanceController.getCurrentRecording().getAllData();

  
  
  
  let oldProfilerData = {
    profilerData: { profile: data.profile },
    ticksData: data.ticks,
    recordingDuration: data.duration,
    fileType: "Recorded Performance Data",
    version: 1
  };

  
  let file = FileUtils.getFile("TmpD", ["tmpprofile.json"]);
  file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));
  yield asyncCopy(oldProfilerData, file);

  

  let rerendered = waitForWidgetsRendered(panel);
  let imported = once(PerformanceController, EVENTS.RECORDING_IMPORTED);
  yield PerformanceController.importRecording("", file);

  yield imported;
  ok(true, "The original profiler data appears to have been successfully imported.");

  yield rerendered;
  ok(true, "The imported data was re-rendered.");

  

  let importedData = PerformanceController.getCurrentRecording().getAllData();

  is(importedData.label, data.label,
    "The imported legacy data was successfully converted for the current tool (1).");
  is(importedData.duration, data.duration,
    "The imported legacy data was successfully converted for the current tool (2).");
  is(importedData.markers.toSource(), [].toSource(),
    "The imported legacy data was successfully converted for the current tool (3).");
  is(importedData.frames.toSource(), [].toSource(),
    "The imported legacy data was successfully converted for the current tool (4).");
  is(importedData.memory.toSource(), [].toSource(),
    "The imported legacy data was successfully converted for the current tool (5).");
  is(importedData.ticks.toSource(), data.ticks.toSource(),
    "The imported legacy data was successfully converted for the current tool (6).");
  is(importedData.allocations.toSource(), ({sites:[], timestamps:[], frames:[], counts:[]}).toSource(),
    "The imported legacy data was successfully converted for the current tool (7).");
  is(importedData.profile.toSource(), data.profile.toSource(),
    "The imported legacy data was successfully converted for the current tool (8).");

  yield teardown(panel);
  finish();
});

function getUnicodeConverter() {
  let className = "@mozilla.org/intl/scriptableunicodeconverter";
  let converter = Cc[className].createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  return converter;
}

function asyncCopy(data, file) {
  let deferred = Promise.defer();

  let string = JSON.stringify(data);
  let inputStream = getUnicodeConverter().convertToInputStream(string);
  let outputStream = FileUtils.openSafeFileOutputStream(file);

  NetUtil.asyncCopy(inputStream, outputStream, status => {
    if (!Components.isSuccessCode(status)) {
      deferred.reject(new Error("Could not save data to file."));
    }
    deferred.resolve();
  });

  return deferred.promise;
}
