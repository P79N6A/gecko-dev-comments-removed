



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");

  let ui = yield openAnimationInspector();
  yield testDataUpdates(ui);

  info("Close the toolbox, reload the tab, and try again with the new UI");
  ui = yield closeAnimationInspectorAndRestartWithNewUI(true);
  yield testDataUpdates(ui, true);
});

function* testDataUpdates({panel, controller, inspector}, isNewUI=false) {
  info("Select the test node");
  yield selectNode(".animated", inspector);

  let animation = controller.animationPlayers[0];
  yield setStyle(animation, "animationDuration", "5.5s", isNewUI);
  yield setStyle(animation, "animationIterationCount", "300", isNewUI);
  yield setStyle(animation, "animationDelay", "45s", isNewUI);

  if (isNewUI) {
    let animationsEl = panel.animationsTimelineComponent.animationsEl;
    let timeBlockEl = animationsEl.querySelector(".time-block");

    
    let expectedTotalDuration = 1695 * 1000;
    let timeRatio = expectedTotalDuration / timeBlockEl.offsetWidth;

    
    
    
    let delayWidth = parseFloat(timeBlockEl.querySelector(".delay").style.width);
    is(Math.round(delayWidth * timeRatio), 45 * 1000,
      "The timeline has the right delay");
  } else {
    let widget = panel.playerWidgets[0];
    is(widget.metaDataComponent.durationValue.textContent, "5.50s",
      "The widget shows the new duration");
    is(widget.metaDataComponent.iterationValue.textContent, "300",
      "The widget shows the new iteration count");
    is(widget.metaDataComponent.delayValue.textContent, "45s",
      "The widget shows the new delay");
  }
}

function* setStyle(animation, name, value, isNewUI=false) {
  info("Change the animation style via the content DOM. Setting " +
    name + " to " + value);

  let onAnimationChanged = once(animation, "changed");
  yield executeInContent("devtools:test:setStyle", {
    selector: ".animated",
    propertyName: name,
    propertyValue: value
  });
  yield onAnimationChanged;

  
  
  if (!isNewUI) {
    yield once(animation, animation.AUTO_REFRESH_EVENT);
  }
}
