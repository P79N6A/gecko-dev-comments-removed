



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select the delayed animation node");
  yield selectNode(".delayed", inspector);

  let widget = panel.playerWidgets[0];

  let timeline = widget.currentTimeEl;
  is(timeline.value, 0, "The timeline is at 0 since the animation hasn't started");

  let timeLabel = widget.timeDisplayEl;
  is(timeLabel.textContent, "0s", "The current time is 0");
});
