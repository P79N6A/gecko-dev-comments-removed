



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select node .still and check that the panel is empty");
  let stillNode = yield getNodeFront(".still", inspector);
  yield selectNode(stillNode, inspector);
  ok(!panel.playerWidgets || !panel.playerWidgets.length,
    "No player widgets displayed for a still node");

  info("Select the comment text node and check that the panel is empty");
  let commentNode = yield inspector.walker.previousSibling(stillNode);
  yield selectNode(commentNode, inspector);
  ok(!panel.playerWidgets || !panel.playerWidgets.length,
    "No player widgets displayed for a text node");
});
