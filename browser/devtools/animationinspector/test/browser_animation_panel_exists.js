



"use strict";



add_task(function*() {
  yield addTab("data:text/html;charset=utf-8,welcome to the animation panel");
  let {panel, controller} = yield openAnimationInspector();

  ok(controller, "The animation controller exists");
  ok(controller.animationsFront, "The animation controller has been initialized");

  ok(panel, "The animation panel exists");
  ok(panel.playersEl, "The animation panel has been initialized");

  ({panel, controller} = yield closeAnimationInspectorAndRestartWithNewUI());

  ok(controller, "The animation controller exists");
  ok(controller.animationsFront, "The animation controller has been initialized");

  ok(panel, "The animation panel exists");
  ok(panel.playersEl, "The animation panel has been initialized");
  ok(panel.animationsTimelineComponent, "The animation panel has been initialized");
});
