



"use strict";




add_task(function*() {
  yield addTab(TEST_URL_ROOT + "doc_simple_animation.html");

  let ui = yield openAnimationInspector();
  yield testTargetNode(ui);

  ui = yield closeAnimationInspectorAndRestartWithNewUI();
  yield testTargetNode(ui, true);
});

function* testTargetNode({toolbox, inspector, panel}, isNewUI) {
  info("Select the simple animated node");
  yield selectNode(".animated", inspector);

  
  
  let targetNodeComponent;
  if (isNewUI) {
    targetNodeComponent = panel.animationsTimelineComponent.targetNodes[0];
  } else {
    targetNodeComponent = panel.playerWidgets[0].targetNodeComponent;
  }
  if (!targetNodeComponent.nodeFront) {
    yield targetNodeComponent.once("target-retrieved");
  }

  info("Retrieve the part of the widget that highlights the node on hover");
  let highlightingEl = targetNodeComponent.previewEl;

  info("Listen to node-highlight event and mouse over the widget");
  let onHighlight = toolbox.once("node-highlight");
  EventUtils.synthesizeMouse(highlightingEl, 10, 5, {type: "mouseover"},
                             highlightingEl.ownerDocument.defaultView);
  let nodeFront = yield onHighlight;

  ok(true, "The node-highlight event was fired");
  is(targetNodeComponent.nodeFront, nodeFront,
    "The highlighted node is the one stored on the animation widget");
  is(nodeFront.tagName, "DIV",
    "The highlighted node has the correct tagName");
  is(nodeFront.attributes[0].name, "class",
    "The highlighted node has the correct attributes");
  is(nodeFront.attributes[0].value, "ball animated",
    "The highlighted node has the correct class");

  info("Select the body node in order to have the list of all animations");
  yield selectNode("body", inspector);

  
  
  if (isNewUI) {
    targetNodeComponent = panel.animationsTimelineComponent.targetNodes[0];
  } else {
    targetNodeComponent = panel.playerWidgets[0].targetNodeComponent;
  }
  if (!targetNodeComponent.nodeFront) {
    yield targetNodeComponent.once("target-retrieved");
  }

  info("Click on the first animation widget's selector icon and wait for the " +
    "selection to change");
  let onSelection = inspector.selection.once("new-node-front");
  let onPanelUpdated = panel.once(panel.UI_UPDATED_EVENT);
  let selectIconEl = targetNodeComponent.selectNodeEl;
  EventUtils.sendMouseEvent({type: "click"}, selectIconEl,
                            selectIconEl.ownerDocument.defaultView);
  yield onSelection;

  is(inspector.selection.nodeFront, targetNodeComponent.nodeFront,
    "The selected node is the one stored on the animation widget");

  yield onPanelUpdated;
}
