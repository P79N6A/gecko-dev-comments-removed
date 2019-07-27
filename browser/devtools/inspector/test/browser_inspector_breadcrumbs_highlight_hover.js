



"use strict";



add_task(function*() {
  info("Loading the test document and opening the inspector");
  let {toolbox, inspector, testActor} = yield openInspectorForURL("data:text/html;charset=utf-8,<h1>foo</h1><span>bar</span>");
  info("Selecting the test node");
  yield selectNode("span", inspector);
  let bcButtons = inspector.breadcrumbs["container"];

  let onNodeHighlighted = toolbox.once("node-highlight");
  let button = bcButtons.childNodes[1];
  EventUtils.synthesizeMouseAtCenter(button, {type: "mousemove"}, button.ownerDocument.defaultView);
  yield onNodeHighlighted;

  let isVisible = yield testActor.isHighlighting();
  ok(isVisible, "The highlighter is shown on a markup container hover");

  ok((yield testActor.assertHighlightedNode("body")), "The highlighter highlights the right node");

  onNodeHighlighted = toolbox.once("node-highlight");
  button = bcButtons.childNodes[2];
  EventUtils.synthesizeMouseAtCenter(button, {type: "mousemove"}, button.ownerDocument.defaultView);
  yield onNodeHighlighted;

  isVisible = yield testActor.isHighlighting();
  ok(isVisible, "The highlighter is shown on a markup container hover");

  ok((yield testActor.assertHighlightedNode("span")), "The highlighter highlights the right node");
});
