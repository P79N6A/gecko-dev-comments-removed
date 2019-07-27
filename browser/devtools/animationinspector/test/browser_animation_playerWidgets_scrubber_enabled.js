



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {controller, inspector, panel} = yield openAnimationInspector();

  info("Select the animated node");
  yield selectNode(".animated", inspector);

  info("Get the player widget's timeline element");
  let widget = panel.playerWidgets[0];
  let timeline = widget.currentTimeEl;

  ok(!timeline.hasAttribute("disabled"), "The timeline input[range] is enabled");
  ok(widget.setCurrentTime, "The widget has the setCurrentTime method");
  ok(widget.player.setCurrentTime, "The associated player front has the setCurrentTime method");

  info("Faking an older server version by setting " +
    "AnimationsController.hasSetCurrentTime to false");

  yield selectNode("body", inspector);
  controller.hasSetCurrentTime = false;

  yield selectNode(".animated", inspector);

  info("Get the player widget's timeline element");
  widget = panel.playerWidgets[0];
  timeline = widget.currentTimeEl;

  ok(timeline.hasAttribute("disabled"), "The timeline input[range] is disabled");

  yield selectNode("body", inspector);
  controller.hasSetCurrentTime = true;
});
