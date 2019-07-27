



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_body_animation.html");

  let {panel} = yield openAnimationInspector();
  is(panel.playerWidgets.length, 1,
    "One animation player is displayed after init");

  ({panel} = yield closeAnimationInspectorAndRestartWithNewUI());
  is(panel.animationsTimelineComponent.animations.length, 1,
    "One animation is handled by the timeline after init");
  is(panel.animationsTimelineComponent.animationsEl.childNodes.length, 1,
    "One animation is displayed after init");
});
