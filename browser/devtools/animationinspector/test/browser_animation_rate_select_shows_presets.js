



"use strict";





add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Selecting the test node");
  yield selectNode(".animated", inspector);

  info("Get the playback rate UI component");
  let widget = panel.playerWidgets[0];
  let rateComponent = widget.rateComponent;

  let options = rateComponent.el.querySelectorAll("option");
  is(options.length, rateComponent.PRESETS.length,
    "The playback rate select contains the right number of options");

  for (let i = 0; i < rateComponent.PRESETS.length; i ++) {
    is(options[i].value, rateComponent.PRESETS[i] + "",
      "The playback rate option " + i + " has the right preset value " +
      rateComponent.PRESETS[i]);
  }

  info("Set a custom rate (not part of the presets) via the DOM");
  let onRateChanged = waitForStateCondition(widget.player, state => {
    return state.playbackRate === 3.6
  });
  yield executeInContent("Test:SetAnimationPlayerPlaybackRate", {
    selector: ".animated",
    animationIndex: 0,
    playbackRate: 3.6
  });
  yield onRateChanged;

  options = rateComponent.el.querySelectorAll("option");
  is(options.length, rateComponent.PRESETS.length + 1,
    "The playback rate select contains the right number of options (presets + 1)");

  ok([...options].find(option => option.value === "3.6"),
    "The custom rate is part of the select");
});
