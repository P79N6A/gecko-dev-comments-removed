



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select node .animated and check that the panel is not empty");
  let node = yield getNodeFront(".animated", inspector);
  yield selectNode(node, inspector);

  is(panel.playerWidgets.length, 1,
    "Exactly 1 player widget is shown for animated node");
});
