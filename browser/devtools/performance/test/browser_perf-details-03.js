







function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, OverviewView, DetailsView } = panel.panelWin;
  let { $, RecordingsView, WaterfallView, MemoryCallTreeView, MemoryFlameGraphView } = panel.panelWin;

  Services.prefs.setBoolPref(MEMORY_PREF, false);
  yield startRecording(panel);
  yield stopRecording(panel);

  ok(DetailsView.isViewSelected(WaterfallView),
    "The waterfall view is selected by default in the details view.");

  Services.prefs.setBoolPref(MEMORY_PREF, true);
  
  
  yield startRecording(panel);
  yield stopRecording(panel);

  let flameBtn = $("toolbarbutton[data-view='memory-flamegraph']");
  let callBtn = $("toolbarbutton[data-view='memory-calltree']");

  is(flameBtn.hidden, false, "memory-flamegraph button shown when recording has memory data");
  is(callBtn.hidden, false, "memory-calltree button shown when recording has memory data");

  let selected = DetailsView.whenViewSelected(MemoryCallTreeView);
  let notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  DetailsView.selectView("memory-calltree");
  yield Promise.all([selected, notified]);

  ok(DetailsView.isViewSelected(MemoryCallTreeView),
    "The memory call tree view can now be selected.");

  selected = DetailsView.whenViewSelected(MemoryFlameGraphView);
  notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  DetailsView.selectView("memory-flamegraph");
  yield Promise.all([selected, notified]);

  ok(DetailsView.isViewSelected(MemoryFlameGraphView),
    "The memory flamegraph view can now be selected.");

  
  selected = DetailsView.whenViewSelected(WaterfallView);
  notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  RecordingsView.selectedIndex = 0;
  yield Promise.all([selected, notified]);

  ok(DetailsView.isViewSelected(WaterfallView),
    "The waterfall view is now selected when switching back to a recording that does not have memory data");
  is(flameBtn.hidden, true, "memory-flamegraph button hidden when recording does not have memory data");
  is(callBtn.hidden, true, "memory-calltree button hidden when recording does not have memory data");

  
  let render = WaterfallView.once(EVENTS.WATERFALL_RENDERED);
  RecordingsView.selectedIndex = 1;
  yield render;

  selected = DetailsView.whenViewSelected(MemoryCallTreeView);
  notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  DetailsView.selectView("memory-calltree");
  yield Promise.all([selected, notified]);

  ok(DetailsView.isViewSelected(MemoryCallTreeView),
    "The memory call tree view can be selected again after going back to the view with memory data");
  is(flameBtn.hidden, false, "memory-flamegraph button shown when recording has memory data");
  is(callBtn.hidden, false, "memory-calltree button shown when recording has memory data");

  yield teardown(panel);
  finish();
}
