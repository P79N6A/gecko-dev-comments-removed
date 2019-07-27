



"use strict";




let L10N = new ViewHelpers.L10N();

add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Selecting the test node");
  yield selectNode(".animated", inspector);

  info("Get the player widget");
  let widget = panel.playerWidgets[0];

  info("Pause the player so we can compare times easily");
  yield executeInContent("Test:ToggleAnimationPlayer", {
    selector: ".animated",
    animationIndex: 0,
    pause: true
  });
  yield widget.player.once(widget.player.AUTO_REFRESH_EVENT);

  ok(widget.el.classList.contains("paused"), "The widget is in pause mode");

  info("Change the animation's currentTime via the content DOM");
  yield executeInContent("Test:SetAnimationPlayerCurrentTime", {
    selector: ".animated",
    animationIndex: 0,
    currentTime: 0
  });
  yield widget.player.once(widget.player.AUTO_REFRESH_EVENT);

  is(widget.currentTimeEl.value, 0, "The currentTime slider's value was changed");

  info("Change the animation's currentTime again via the content DOM");
  yield executeInContent("Test:SetAnimationPlayerCurrentTime", {
    selector: ".animated",
    animationIndex: 0,
    currentTime: 300
  });
  yield widget.player.once(widget.player.AUTO_REFRESH_EVENT);

  is(widget.currentTimeEl.value, 300,
    "The currentTime slider's value was changed again");
});
