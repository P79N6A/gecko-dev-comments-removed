



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");

  let {inspector, panel} = yield openAnimationInspector();
  yield testEmptyPanel(inspector, panel);

  ({inspector, panel}) = yield closeAnimationInspectorAndRestartWithNewUI();
  yield testEmptyPanel(inspector, panel, true);
});

function* testEmptyPanel(inspector, panel, isNewUI=false) {
  info("Select node .still and check that the panel is empty");
  let stillNode = yield getNodeFront(".still", inspector);
  let onUpdated = panel.once(panel.UI_UPDATED_EVENT);
  yield selectNode(stillNode, inspector);
  yield onUpdated;

  if (isNewUI) {
    is(panel.animationsTimelineComponent.animations.length, 0,
       "No animation players stored in the timeline component for a still node");
    is(panel.animationsTimelineComponent.animationsEl.childNodes.length, 0,
       "No animation displayed in the timeline component for a still node");
  } else {
    ok(!panel.playerWidgets || !panel.playerWidgets.length,
       "No player widgets displayed for a still node");
  }

  info("Select the comment text node and check that the panel is empty");
  let commentNode = yield inspector.walker.previousSibling(stillNode);
  onUpdated = panel.once(panel.UI_UPDATED_EVENT);
  yield selectNode(commentNode, inspector);
  yield onUpdated;

  if (isNewUI) {
    is(panel.animationsTimelineComponent.animations.length, 0,
       "No animation players stored in the timeline component for a text node");
    is(panel.animationsTimelineComponent.animationsEl.childNodes.length, 0,
       "No animation displayed in the timeline component for a text node");
  } else {
    ok(!panel.playerWidgets || !panel.playerWidgets.length,
       "No player widgets displayed for a text node");
  }
}
