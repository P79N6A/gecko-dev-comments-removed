



"use strict";





let test = asyncTest(function*() {
  let {inspector} = yield addTab("data:text/html,<p>Select me!</p>").then(openInspector);

  info("hover over the <p> line in the markup-view so that it's the currently hovered node");
  yield hoverContainer("p", inspector);

  info("select the <p> markup-container line by clicking");
  yield clickContainer("p", inspector);
  ok(isHighlighterVisible(), "the highlighter is shown");

  info("mouse-leave the markup-view");
  yield mouseLeaveMarkupView(inspector);
  ok(!isHighlighterVisible(), "the highlighter is hidden after mouseleave");

  info("hover over the <p> line again, which is still selected");
  yield hoverContainer("p", inspector);
  ok(isHighlighterVisible(), "the highlighter is visible again");
});
