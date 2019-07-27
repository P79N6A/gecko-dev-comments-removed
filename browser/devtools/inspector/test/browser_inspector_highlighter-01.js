



"use strict";



let test = asyncTest(function*() {
  info("Loading the test document and opening the inspector");
  yield addTab("data:text/html;charset=utf-8,<h1>foo</h1><span>bar</span>");
  let {toolbox, inspector} = yield openInspector();

  info("Selecting the test node");
  yield selectNode("span", inspector);

  let container = getContainerForRawNode(inspector.markup, getNode("h1"));

  let onHighlighterReady = toolbox.once("highlighter-ready");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousemove"},
                                     inspector.markup.doc.defaultView);
  yield onHighlighterReady;

  ok(isHighlighting(), "The highlighter is shown on a markup container hover");
  is(getHighlitNode(), getNode("h1"), "The highlighter highlights the right node");

  gBrowser.removeCurrentTab();
});
