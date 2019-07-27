




"use strict";




const NESTED_FRAME_SRC = "data:text/html;charset=utf-8," +
  "nested iframe<div>nested div</div>";

const OUTER_FRAME_SRC = "data:text/html;charset=utf-8," +
  "little frame<div>little div</div>" +
  "<iframe src='" + NESTED_FRAME_SRC + "' />";

const TEST_URI = "data:text/html;charset=utf-8," +
  "iframe tests for inspector" +
  "<iframe src=\"" + OUTER_FRAME_SRC + "\" />";

add_task(function*() {
  let {toolbox, inspector} = yield openInspectorForURL(TEST_URI);
  let outerFrame = getNode("iframe");
  let outerFrameDiv = getNode("div", { document: outerFrame.contentDocument});
  let innerFrame = getNode("iframe", { document: outerFrame.contentDocument});
  let innerFrameDiv = getNode("div", { document: innerFrame.contentDocument});

  info("Waiting for element picker to activate.");
  yield inspector.toolbox.highlighterUtils.startPicker();

  info("Moving mouse over outerFrameDiv");
  yield moveMouseOver(outerFrameDiv);
  let highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode, outerFrameDiv, "outerFrameDiv is highlighted.");

  info("Moving mouse over innerFrameDiv");
  yield moveMouseOver(innerFrameDiv);
  highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode, innerFrameDiv, "innerFrameDiv is highlighted.");

  info("Selecting root node");
  yield selectNode(inspector.walker.rootNode, inspector);

  info("Selecting an element from the nested iframe directly");
  let innerFrameFront = yield getNodeFrontInFrame("iframe", "iframe", inspector);
  let innerFrameDivFront = yield getNodeFrontInFrame("div", innerFrameFront, inspector);
  yield selectNode(innerFrameDivFront, inspector);

  is(inspector.breadcrumbs.nodeHierarchy.length, 9, "Breadcrumbs have 9 items.");

  info("Waiting for element picker to deactivate.");
  yield inspector.toolbox.highlighterUtils.stopPicker();

  function moveMouseOver(node) {
    info("Waiting for element " + node + " to be highlighted");
    executeInContent("Test:SynthesizeMouse", {
      options: {type: "mousemove"},
      center: true
    }, {node}, false);
    return inspector.toolbox.once("picker-node-hovered");
  }
});
