



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Selecting a node with an animation that doesn't repeat");
  yield selectNode(".long", inspector);
  let widget = panel.playerWidgets[0];
  let metaDataLabels = widget.el.querySelectorAll(".animation-title .meta-data strong");
  is(metaDataLabels.length, 1, "Only the duration is shown");

  info("Selecting a node with an animation that repeats several times");
  yield selectNode(".delayed", inspector);
  widget = panel.playerWidgets[0];
  let iterationLabel = widget.el.querySelectorAll(".animation-title .meta-data strong")[2];
  is(iterationLabel.textContent, "10", "The iteration is shown");
});
