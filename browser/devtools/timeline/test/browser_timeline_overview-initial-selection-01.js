







let test = Task.async(function*() {
  let [target, debuggee, panel] = yield initTimelinePanel(SIMPLE_URL);
  let { EVENTS, TimelineView, TimelineController } = panel.panelWin;
  let { OVERVIEW_INITIAL_SELECTION_RATIO: selectionRatio } = panel.panelWin;

  yield TimelineController.toggleRecording();
  ok(true, "Recording has started.");

  let updated = 0;
  panel.panelWin.on(EVENTS.OVERVIEW_UPDATED, () => updated++);

  ok((yield waitUntil(() => updated > 10)),
    "The overview graph was updated a bunch of times.");
  ok((yield waitUntil(() => TimelineController.getMarkers().length > 0)),
    "There are some markers available.");

  yield TimelineController.toggleRecording();
  ok(true, "Recording has ended.");

  let markers = TimelineController.getMarkers();
  let selection = TimelineView.overview.getSelection();

  is(selection.start, markers[0].start * TimelineView.overview.dataScaleX,
    "The initial selection start is correct.");
  is(selection.end - selection.start, TimelineView.overview.width * selectionRatio,
    "The initial selection end is correct.");

  yield teardown(panel);
  finish();
});
