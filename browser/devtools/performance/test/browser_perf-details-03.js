


let MEMORY_PREF = "devtools.performance.ui.enable-memory";





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, DetailsView } = panel.panelWin;
  let { $, WaterfallView, MemoryCallTreeView, MemoryFlameGraphView } = panel.panelWin;

  Services.prefs.setBoolPref(MEMORY_PREF, false);

  ok(DetailsView.isViewSelected(WaterfallView),
    "The waterfall view is selected by default in the details view.");

  let flameBtn = $("toolbarbutton[data-view='memory-flamegraph']");
  let callBtn = $("toolbarbutton[data-view='memory-calltree']");

  is(flameBtn.hidden, true, "memory-flamegraph button hidden when enable-memory=false");
  is(callBtn.hidden, true, "memory-calltree button hidden when enable-memory=false");

  Services.prefs.setBoolPref(MEMORY_PREF, true);

  is(flameBtn.hidden, false, "memory-flamegraph button shown when enable-memory=true");
  is(callBtn.hidden, false, "memory-calltree button shown when enable-memory=true");

  let selected = DetailsView.whenViewSelected(MemoryCallTreeView);
  let notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  DetailsView.selectView("memory-calltree");
  yield Promise.all([selected, notified]);

  selected = DetailsView.whenViewSelected(WaterfallView);
  notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  Services.prefs.setBoolPref(MEMORY_PREF, false);
  yield Promise.all([selected, notified]);

  ok(DetailsView.isViewSelected(WaterfallView),
    "The waterfall view is now selected when toggling off enable-memory when a memory panel is selected.");

  Services.prefs.setBoolPref(MEMORY_PREF, true);

  selected = DetailsView.whenViewSelected(MemoryFlameGraphView);
  notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  DetailsView.selectView("memory-flamegraph");
  yield Promise.all([selected, notified]);

  selected = DetailsView.whenViewSelected(WaterfallView);
  notified = DetailsView.once(EVENTS.DETAILS_VIEW_SELECTED);
  Services.prefs.setBoolPref(MEMORY_PREF, false);
  yield Promise.all([selected, notified]);

  yield teardown(panel);
  finish();
}
