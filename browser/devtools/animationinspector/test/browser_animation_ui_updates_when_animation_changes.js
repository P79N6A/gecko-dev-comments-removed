



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {panel, inspector} = yield openAnimationInspector();

  info("Select the test node");
  yield selectNode(".animated", inspector);

  info("Get the player widget");
  let widget = panel.playerWidgets[0];

  info("Pause the animation via the content DOM");
  yield executeInContent("Test:ToggleAnimationPlayer", {
    animationIndex: 0,
    pause: true
  }, {
    node: getNode(".animated")
  });

  info("Wait for the next state update");
  yield widget.player.once(widget.player.AUTO_REFRESH_EVENT);

  is(widget.player.state.playState, "paused", "The AnimationPlayerFront is paused");
  ok(widget.el.classList.contains("paused"), "The button's state has changed");
  ok(!widget.rafID, "The smooth timeline animation has been stopped");

  info("Play the animation via the content DOM");
  yield executeInContent("Test:ToggleAnimationPlayer", {
    animationIndex: 0,
    pause: false
  }, {
    node: getNode(".animated")
  });

  info("Wait for the next state update");
  yield widget.player.once(widget.player.AUTO_REFRESH_EVENT);

  is(widget.player.state.playState, "running", "The AnimationPlayerFront is running");
  ok(widget.el.classList.contains("running"), "The button's state has changed");
  ok(widget.rafID, "The smooth timeline animation has been started");
});
