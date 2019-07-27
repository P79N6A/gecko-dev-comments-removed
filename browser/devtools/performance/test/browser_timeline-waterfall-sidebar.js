






function* spawnTest() {
  let { target, panel } = yield initPerformance(SIMPLE_URL);
  let { $, $$, PerformanceController, WaterfallView } = panel.panelWin;
  let { L10N } = devtools.require("devtools/performance/global");
  let { getMarkerLabel } = devtools.require("devtools/performance/marker-utils");

  
  
  
  
  WaterfallView._prepareWaterfallTree = markers => {
    return { submarkers: markers };
  };

  yield startRecording(panel);
  ok(true, "Recording has started.");

  yield waitUntil(() => {
    
    let markers = PerformanceController.getCurrentRecording().getMarkers();
    return markers.some(m => m.name == "Styles") &&
           markers.some(m => m.name == "Reflow") &&
           markers.some(m => m.name == "Paint");
  });

  yield stopRecording(panel);
  ok(true, "Recording has ended.");

  let bars = $$(".waterfall-marker-bar");
  let markers = PerformanceController.getCurrentRecording().getMarkers();

  ok(bars.length > 2, "Got at least 3 markers (1)");
  ok(markers.length > 2, "Got at least 3 markers (2)");

  let toMs = ms => L10N.getFormatStrWithNumbers("timeline.tick", ms);

  for (let i = 0; i < bars.length; i++) {
    let bar = bars[i];
    let mkr = markers[i];
    EventUtils.sendMouseEvent({ type: "mousedown" }, bar);

    let type = $(".marker-details-type").getAttribute("value");
    let tooltip = $(".marker-details-duration").getAttribute("tooltiptext");
    let duration = $(".marker-details-duration .marker-details-labelvalue").getAttribute("value");

    info("Current marker data: " + mkr.toSource());
    info("Current marker output: " + $("#waterfall-details").innerHTML);

    is(type, getMarkerLabel(mkr), "Sidebar title matches markers name.");

    
    is(toMs(mkr.end - mkr.start), duration, "Sidebar duration is valid.");

    
    
    ok(tooltip.indexOf(toMs(mkr.start)) !== -1, "Tooltip has start time.");
    ok(tooltip.indexOf(toMs(mkr.end)) !== -1, "Tooltip has end time.");
  }

  yield teardown(panel);
  finish();
}
