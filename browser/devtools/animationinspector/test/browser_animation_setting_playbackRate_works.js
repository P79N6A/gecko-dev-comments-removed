



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {toolbox, inspector, panel} = yield openAnimationInspector();

  info("Select an animated node");
  yield selectNode(".animated", inspector);

  info("Get the player widget for this node");
  let widget = panel.playerWidgets[0];
  let select = widget.rateComponent.el;
  let win = select.ownerDocument.defaultView;

  info("Click on the rate drop-down");
  EventUtils.synthesizeMouseAtCenter(select, {type: "mousedown"}, win);

  info("Click on a rate option");
  let option = select.options[select.options.length - 1];
  EventUtils.synthesizeMouseAtCenter(option, {type: "mouseup"}, win);
  let selectedRate = parseFloat(option.value);

  info("Check that the rate was changed on the player at the next update");
  yield waitForStateCondition(widget.player, ({playbackRate}) => playbackRate === selectedRate);
  is(widget.player.state.playbackRate, selectedRate,
    "The rate was changed successfully");
});
