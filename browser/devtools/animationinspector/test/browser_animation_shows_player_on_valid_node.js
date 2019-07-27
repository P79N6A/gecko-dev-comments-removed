



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");

  let {inspector, panel} = yield openAnimationInspector();
  yield testShowsAnimations(inspector, panel);

  ({inspector, panel}) = yield closeAnimationInspectorAndRestartWithNewUI();
  yield testShowsAnimations(inspector, panel);
});

function* testShowsAnimations(inspector, panel) {
  info("Select node .animated and check that the panel is not empty");
  let node = yield getNodeFront(".animated", inspector);
  yield selectNode(node, inspector);

  assertAnimationsDisplayed(panel, 1);
}
