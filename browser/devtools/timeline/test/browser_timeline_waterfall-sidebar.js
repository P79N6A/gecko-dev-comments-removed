






let test = Task.async(function*() {
  let { target, panel } = yield initTimelinePanel(SIMPLE_URL);
  let { $, $$, EVENTS, TimelineController, TimelineView } = panel.panelWin;
  let { L10N } = devtools.require("devtools/timeline/global");

  yield TimelineController.toggleRecording();
  ok(true, "Recording has started.");

  yield waitUntil(() => {
    
    let markers = TimelineController.getMarkers();
    return markers.some(m => m.name == "Styles") &&
           markers.some(m => m.name == "Reflow") &&
           markers.some(m => m.name == "Paint");
  });

  yield TimelineController.toggleRecording();
  ok(true, "Recording has ended.");

  
  TimelineView.markersOverview.setSelection({ start: 0, end: TimelineView.markersOverview.width })


  let bars = $$(".waterfall-marker-item:not(spacer) > .waterfall-marker-bar");
  let markers = TimelineController.getMarkers();

  ok(bars.length > 2, "got at least 3 markers");

  let sidebar = $("#timeline-waterfall-details");
  for (let i = 0; i < bars.length; i++) {
    let bar = bars[i];
    bar.click();
    let m = markers[i];

    is($("#timeline-waterfall-details .marker-details-type").getAttribute("value"), m.name,
      "sidebar title matches markers name");

    let printedStartTime = $(".marker-details-start .marker-details-labelvalue").getAttribute("value");
    let printedEndTime = $(".marker-details-end .marker-details-labelvalue").getAttribute("value");
    let printedDuration= $(".marker-details-duration .marker-details-labelvalue").getAttribute("value");

    let toMs = ms => L10N.getFormatStrWithNumbers("timeline.tick", ms);

    
    is(toMs(m.start), printedStartTime, "sidebar start time is valid");
    is(toMs(m.end), printedEndTime, "sidebar end time is valid");
    is(toMs(m.end - m.start), printedDuration, "sidebar duration is valid");
  }

  yield teardown(panel);
  finish();
});
