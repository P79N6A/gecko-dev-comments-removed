





function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, OverviewView } = panel.panelWin;
  let framerateGraph = OverviewView.framerateGraph;
  let markersOverview = OverviewView.markersOverview;
  let memoryOverview = OverviewView.memoryOverview;

  let MAX = framerateGraph.width;

  yield startRecording(panel);
  yield stopRecording(panel);

  

  let selected = once(OverviewView, EVENTS.OVERVIEW_RANGE_SELECTED);
  dragStart(framerateGraph, 0);
  dragStop(framerateGraph, MAX / 2);
  yield selected;

  is(framerateGraph.getSelection().toSource(), "({start:0, end:" + (MAX / 2) + "})",
    "The framerate graph has a correct selection.");
  is(markersOverview.getSelection().toSource(), framerateGraph.getSelection().toSource(),
    "The markers overview has a correct selection.");
  is(memoryOverview.getSelection().toSource(), framerateGraph.getSelection().toSource(),
    "The memory overview has a correct selection.");

  

  selected = once(OverviewView, EVENTS.OVERVIEW_RANGE_SELECTED);
  markersOverview.dropSelection();
  dragStart(markersOverview, 0);
  dragStop(markersOverview, MAX / 4);
  yield selected;

  is(framerateGraph.getSelection().toSource(), "({start:0, end:" + (MAX / 4) + "})",
    "The framerate graph has a correct selection.");
  is(markersOverview.getSelection().toSource(), framerateGraph.getSelection().toSource(),
    "The markers overview has a correct selection.");
  is(memoryOverview.getSelection().toSource(), framerateGraph.getSelection().toSource(),
    "The memory overview has a correct selection.");

  

  selected = once(OverviewView, EVENTS.OVERVIEW_RANGE_SELECTED);
  memoryOverview.dropSelection();
  dragStart(memoryOverview, 0);
  dragStop(memoryOverview, MAX / 10);
  yield selected;

  is(framerateGraph.getSelection().toSource(), "({start:0, end:" + (MAX / 10) + "})",
    "The framerate graph has a correct selection.");
  is(markersOverview.getSelection().toSource(), framerateGraph.getSelection().toSource(),
    "The markers overview has a correct selection.");
  is(memoryOverview.getSelection().toSource(), framerateGraph.getSelection().toSource(),
    "The memory overview has a correct selection.");

  yield teardown(panel);
  finish();
}
