



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspectorNewUI();

  info("Selecting a delayed animated node");
  yield selectNode(".delayed", inspector);

  info("Getting the animation and delay elements from the panel");
  let timelineEl = panel.animationsTimelineComponent.rootWrapperEl;
  let delay = timelineEl.querySelector(".delay");

  ok(delay, "The animation timeline contains the delay element");

  info("Selecting a no-delay animated node");
  yield selectNode(".animated", inspector);

  info("Getting the animation and delay elements from the panel again");
  delay = timelineEl.querySelector(".delay");

  ok(!delay, "The animation timeline contains no delay element");
});
