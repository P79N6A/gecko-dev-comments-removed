



"use strict";



add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");
  let {inspector, panel} = yield openAnimationInspector();

  info("Select the simple animated node");
  yield selectNode(".animated", inspector);

  let widget = panel.playerWidgets[0];

  
  
  if (!widget.targetNodeComponent.nodeFront) {
    yield widget.targetNodeComponent.once("target-retrieved");
  }

  let targetEl = widget.el.querySelector(".animation-target");
  ok(targetEl, "The player widget has a target element");
  is(targetEl.textContent, "<divid=\"\"class=\"ball animated\">",
    "The target element's content is correct");

  let selectorEl = targetEl.querySelector(".node-selector");
  ok(selectorEl,
    "The icon to select the target element in the inspector exists");

  info("Test again with the new timeline UI");
  ({inspector, panel} = yield closeAnimationInspectorAndRestartWithNewUI());

  info("Select the simple animated node");
  yield selectNode(".animated", inspector);

  let targetNodeComponent = panel.animationsTimelineComponent.targetNodes[0];
  
  
  if (!targetNodeComponent.nodeFront) {
    yield targetNodeComponent.once("target-retrieved");
  }

  is(targetNodeComponent.el.textContent, "div#.ball.animated",
    "The target element's content is correct");

  selectorEl = targetNodeComponent.el.querySelector(".node-selector");
  ok(selectorEl,
    "The icon to select the target element in the inspector exists");
});
