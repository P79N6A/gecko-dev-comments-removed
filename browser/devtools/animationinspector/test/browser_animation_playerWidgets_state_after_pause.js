



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Selecting the test node");
  yield selectNode(".animated", inspector);

  info("Pausing the animation by using the widget");
  let widget = panel.playerWidgets[0];
  yield widget.pause();

  info("Selecting another node and then the same node again to refresh the widget");
  yield selectNode(".still", inspector);
  yield selectNode(".animated", inspector);

  widget = panel.playerWidgets[0];
  ok(widget.el.classList.contains("paused"), "The widget is still in paused mode");
  is(widget.timeDisplayEl.textContent,
    widget.getFormattedTime(widget.player.state.currentTime) + "s",
    "The initial time has been set to the player's");
});
