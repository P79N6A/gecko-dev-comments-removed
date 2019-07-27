



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select an animated node");
  yield selectNode(".multi", inspector);

  info("Hold on to one of the player widget instances to test it after destroy");
  let widget = panel.playerWidgets[0];

  info("Select another node to get the previous widgets destroyed");
  yield selectNode(".animated", inspector);

  ok(widget.destroyed, "The widget's destroyed flag is true");
});
