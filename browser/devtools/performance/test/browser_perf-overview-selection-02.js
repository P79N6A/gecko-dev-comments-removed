





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, OverviewView } = panel.panelWin;

  
  Services.prefs.setBoolPref(MEMORY_PREF, true);

  yield startRecording(panel);

  yield Promise.all([
    once(OverviewView, EVENTS.FRAMERATE_GRAPH_RENDERED),
    once(OverviewView, EVENTS.MARKERS_GRAPH_RENDERED),
    once(OverviewView, EVENTS.MEMORY_GRAPH_RENDERED),
    once(OverviewView, EVENTS.OVERVIEW_RENDERED),
  ]);

  let markersOverview = OverviewView.markersOverview;
  let memoryOverview = OverviewView.memoryOverview;
  let framerateGraph = OverviewView.framerateGraph;

  ok(markersOverview,
    "The markers graph should have been created now.");
  ok(memoryOverview,
    "The memory graph should have been created now.");
  ok(framerateGraph,
    "The framerate graph should have been created now.");

  ok(!framerateGraph.selectionEnabled,
    "Selection shouldn't be enabled when the first recording started (1).");
  ok(!markersOverview.selectionEnabled,
    "Selection shouldn't be enabled when the first recording started (2).");
  ok(!memoryOverview.selectionEnabled,
    "Selection shouldn't be enabled when the first recording started (3).");

  yield stopRecording(panel);

  ok(framerateGraph.selectionEnabled,
    "Selection should be enabled when the first recording finishes (1).");
  ok(markersOverview.selectionEnabled,
    "Selection should be enabled when the first recording finishes (2).");
  ok(memoryOverview.selectionEnabled,
    "Selection should be enabled when the first recording finishes (3).");

  yield startRecording(panel);

  yield Promise.all([
    once(OverviewView, EVENTS.FRAMERATE_GRAPH_RENDERED),
    once(OverviewView, EVENTS.MARKERS_GRAPH_RENDERED),
    once(OverviewView, EVENTS.MEMORY_GRAPH_RENDERED),
    once(OverviewView, EVENTS.OVERVIEW_RENDERED),
  ]);

  ok(!framerateGraph.selectionEnabled,
    "Selection shouldn't be enabled when the second recording started (1).");
  ok(!markersOverview.selectionEnabled,
    "Selection shouldn't be enabled when the second recording started (2).");
  ok(!memoryOverview.selectionEnabled,
    "Selection shouldn't be enabled when the second recording started (3).");

  yield stopRecording(panel);

  ok(framerateGraph.selectionEnabled,
    "Selection should be enabled when the first second finishes (1).");
  ok(markersOverview.selectionEnabled,
    "Selection should be enabled when the first second finishes (2).");
  ok(memoryOverview.selectionEnabled,
    "Selection should be enabled when the first second finishes (3).");

  yield teardown(panel);
  finish();
}
