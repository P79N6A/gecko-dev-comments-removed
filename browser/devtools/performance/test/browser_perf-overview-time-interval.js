






function spawnTest () {
  let { panel } = yield initPerformance(SIMPLE_URL);
  let { EVENTS, PerformanceController, OverviewView } = panel.panelWin;

  try {
    OverviewView.setTimeInterval({ starTime: 0, endTime: 1 });
    ok(false, "Setting a time interval shouldn't have worked.");
  } catch (e) {
    ok(true, "Setting a time interval didn't work, as expected.");
  }

  try {
    OverviewView.getTimeInterval();
    ok(false, "Getting the time interval shouldn't have worked.");
  } catch (e) {
    ok(true, "Getting the time interval didn't work, as expected.");
  }

  yield startRecording(panel);

  yield Promise.all([
    once(OverviewView, EVENTS.FRAMERATE_GRAPH_RENDERED),
    once(OverviewView, EVENTS.MARKERS_GRAPH_RENDERED),
    once(OverviewView, EVENTS.OVERVIEW_RENDERED)
  ]);

  yield stopRecording(panel);

  

  let notified = once(OverviewView, EVENTS.OVERVIEW_RANGE_SELECTED);
  OverviewView.setTimeInterval({ startTime: 10, endTime: 20 });
  yield notified;

  let firstInterval = OverviewView.getTimeInterval();
  ok(firstInterval.startTime - 10 < Number.EPSILON,
    "The interval's start time was properly set.");
  ok(firstInterval.endTime - 20 < Number.EPSILON,
    "The interval's end time was properly set.");

  

  function fail() {
    ok(false, "The selection event should not have propagated.");
  }

  OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, fail);
  OverviewView.setTimeInterval({ startTime: 30, endTime: 40 }, { stopPropagation: true });
  OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, fail);

  let secondInterval = OverviewView.getTimeInterval();
  is(Math.round(secondInterval.startTime), 30,
    "The interval's start time was properly set again.");
  is(Math.round(secondInterval.endTime), 40,
    "The interval's end time was properly set again.");

  yield teardown(panel);
  finish();
}
