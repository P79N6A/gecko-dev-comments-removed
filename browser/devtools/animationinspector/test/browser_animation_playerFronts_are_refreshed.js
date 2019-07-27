



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {controller, inspector} = yield openAnimationInspector();

  info("Selecting an animated node");
  
  
  
  yield selectNode(".animated", inspector);

  is(controller.animationPlayers.length, 1,
    "One AnimationPlayerFront has been created");
  ok(controller.animationPlayers[0].autoRefreshTimer,
    "The AnimationPlayerFront has been set to auto-refresh");

  info("Selecting a node with mutliple animations");
  yield selectNode(".multi", inspector);

  is(controller.animationPlayers.length, 2,
    "2 AnimationPlayerFronts have been created");
  ok(controller.animationPlayers[0].autoRefreshTimer &&
     controller.animationPlayers[1].autoRefreshTimer,
    "The AnimationPlayerFronts have been set to auto-refresh");

  
  
  
  let retainedFront = controller.animationPlayers[0];
  let oldRelease = retainedFront.release;
  let releaseCalled = false;
  retainedFront.release = () => {
    releaseCalled = true;
  };

  info("Selecting a node with no animations");
  yield selectNode(".still", inspector);

  is(controller.animationPlayers.length, 0,
    "There are no more AnimationPlayerFront objects");

  info("Checking the destroyed AnimationPlayerFront object");
  ok(releaseCalled, "The AnimationPlayerFront has been released");
  ok(!retainedFront.autoRefreshTimer,
    "The released AnimationPlayerFront's auto-refresh mode has been turned off");
  yield oldRelease.call(retainedFront);
});
