







let test = Task.async(function*() {
  let { target, panel, toolbox } = yield initPerformance(SIMPLE_URL);
  let { $, EVENTS, PerformanceController, DetailsView, DetailsSubview, RecordingsView } = panel.panelWin;

  
  Services.prefs.setBoolPref(MEMORY_PREF, true);

  
  
  DetailsSubview.canUpdateWhileHidden = true;

  yield startRecording(panel);
  yield stopRecording(panel);

  
  
  
  yield DetailsView.selectView("js-calltree");
  yield DetailsView.selectView("js-flamegraph");
  yield DetailsView.selectView("memory-calltree");
  yield DetailsView.selectView("memory-flamegraph");

  yield startRecording(panel);
  yield stopRecording(panel);

  let rerender = waitForWidgetsRendered(panel);
  RecordingsView.selectedIndex = 0;
  yield rerender;

  rerender = waitForWidgetsRendered(panel);
  RecordingsView.selectedIndex = 1;
  yield rerender;

  yield teardown(panel);
  finish();
});
