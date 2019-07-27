



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Selecting a node with an animation that doesn't repeat");
  yield selectNode(".long", inspector);
  let widget = panel.playerWidgets[0];

  ok(isNodeVisible(widget.metaDataComponent.durationValue),
    "The duration value is shown");
  ok(!isNodeVisible(widget.metaDataComponent.delayValue),
    "The delay value is hidden");
  ok(!isNodeVisible(widget.metaDataComponent.iterationValue),
    "The iteration count is hidden");

  info("Selecting a node with an animation that repeats several times");
  yield selectNode(".delayed", inspector);
  widget = panel.playerWidgets[0];

  ok(isNodeVisible(widget.metaDataComponent.durationValue),
    "The duration value is shown");
  ok(isNodeVisible(widget.metaDataComponent.delayValue),
    "The delay value is shown");
  ok(isNodeVisible(widget.metaDataComponent.iterationValue),
    "The iteration count is shown");
});
