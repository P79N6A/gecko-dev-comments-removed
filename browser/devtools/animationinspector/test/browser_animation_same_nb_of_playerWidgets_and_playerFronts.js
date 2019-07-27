



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel, controller} = yield openAnimationInspector();

  info("Selecting the test animated node");
  yield selectNode(".multi", inspector);

  is(controller.animationPlayers.length, panel.playerWidgets.length,
    "As many playerWidgets were created as there are playerFronts");

  for (let widget of panel.playerWidgets) {
    ok(widget.initialized, "The player widget is initialized");
    is(widget.el.parentNode, panel.playersEl,
      "The player widget has been appended to the panel");
  }

  info("Test again with the new UI, making sure the same number of " +
       "animation timelines is created");
  ({inspector, panel, controller} = yield closeAnimationInspectorAndRestartWithNewUI());
  let timeline = panel.animationsTimelineComponent;

  info("Selecting the test animated node again");
  yield selectNode(".multi", inspector);

  is(controller.animationPlayers.length,
    timeline.animationsEl.querySelectorAll(".animation").length,
    "As many timeline elements were created as there are playerFronts");
});
