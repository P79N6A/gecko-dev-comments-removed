



"use strict";



let test = asyncTest(function*() {
  info("Loading the test document and opening the inspector");
  yield addTab("data:text/html;charset=utf-8,<h1>foo</h1><span>bar</span>");
  let {toolbox, inspector} = yield openInspector();
  info("Selecting the test node");
  yield selectNode("span", inspector);
  let bcButtons = inspector.breadcrumbs["container"];

  let onNodeHighlighted = toolbox.once("node-highlight");
  let button = bcButtons.childNodes[1];
  EventUtils.synthesizeMouseAtCenter(button, {type: "mousemove"}, button.ownerDocument.defaultView);
  yield onNodeHighlighted;
  ok(isHighlighting(), "The highlighter is shown on a markup container hover");
  is(getHighlitNode(), getNode("body"), "The highlighter highlights the right node");

  onNodeHighlighted = toolbox.once("node-highlight");
  button = bcButtons.childNodes[2];
  EventUtils.synthesizeMouseAtCenter(button, {type: "mousemove"}, button.ownerDocument.defaultView);
  yield onNodeHighlighted;
  ok(isHighlighting(), "The highlighter is shown on a markup container hover");
  is(getHighlitNode(), getNode("span"), "The highlighter highlights the right node");

  gBrowser.removeCurrentTab();
});
