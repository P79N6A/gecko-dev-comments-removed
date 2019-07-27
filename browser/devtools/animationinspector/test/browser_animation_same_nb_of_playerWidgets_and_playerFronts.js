



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
});
