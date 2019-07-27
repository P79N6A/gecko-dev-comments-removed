






let test = Task.async(function*() {
  let { target, panel } = yield initTimelinePanel(SIMPLE_URL);
  let { $, gFront, TimelineController } = panel.panelWin;

  $("#memory-checkbox").checked = true;
  yield TimelineController.updateMemoryRecording();

  is((yield gFront.isRecording()), false,
    "The timeline actor should not be recording when the tool starts.");
  is(TimelineController.getMarkers().length, 0,
    "There should be no markers available when the tool starts.");

  yield TimelineController.toggleRecording();

  is((yield gFront.isRecording()), true,
    "The timeline actor should be recording now.");
  ok((yield waitUntil(() => TimelineController.getMarkers().length > 0)),
    "There are some markers available now.");
  ok((yield waitUntil(() => TimelineController.getMemory().length > 0)),
    "There are some memory measurements available now.");

  ok("startTime" in TimelineController.getInterval(),
    "A `startTime` field was set on the recording data.");
  ok("endTime" in TimelineController.getInterval(),
    "An `endTime` field was set on the recording data.");

  ok(TimelineController.getInterval().endTime >
     TimelineController.getInterval().startTime,
    "Some time has passed since the recording started.");

  yield teardown(panel);
  finish();
});
