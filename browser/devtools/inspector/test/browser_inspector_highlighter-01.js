



"use strict";



add_task(function*() {
  info("Loading the test document and opening the inspector");
  let {toolbox, inspector, testActor} = yield openInspectorForURL("data:text/html;charset=utf-8,<h1>foo</h1><span>bar</span>");

  let isVisible = yield testActor.isHighlighting(toolbox);
  ok(!isVisible, "The highlighter is hidden by default");

  info("Selecting the test node");
  yield selectNode("span", inspector);
  let container = yield getContainerForSelector("h1", inspector);

  let onHighlighterReady = toolbox.once("highlighter-ready");
  EventUtils.synthesizeMouseAtCenter(container.tagLine, {type: "mousemove"},
                                     inspector.markup.doc.defaultView);
  yield onHighlighterReady;

  isVisible = yield testActor.isHighlighting();
  ok(isVisible, "The highlighter is shown on a markup container hover");

  ok((yield testActor.assertHighlightedNode("h1")), "The highlighter highlights the right node");
});
