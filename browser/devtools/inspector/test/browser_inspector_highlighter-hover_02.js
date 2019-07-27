



"use strict";





const TEST_URL = "data:text/html;charset=utf-8,<p>Select me!</p>";

add_task(function*() {
  let {toolbox, inspector} = yield openInspectorForURL(TEST_URL);

  info("hover over the <p> line in the markup-view so that it's the currently hovered node");
  yield hoverContainer("p", inspector);

  info("select the <p> markup-container line by clicking");
  yield clickContainer("p", inspector);
  let isVisible = yield isHighlighting(toolbox);
  ok(isVisible, "the highlighter is shown");

  info("listen to the highlighter's hidden event");
  let onHidden = waitForHighlighterEvent("hidden", toolbox.highlighter);
  info("mouse-leave the markup-view");
  yield mouseLeaveMarkupView(inspector);
  yield onHidden;
  isVisible = yield isHighlighting(toolbox);
  ok(!isVisible, "the highlighter is hidden after mouseleave");

  info("hover over the <p> line again, which is still selected");
  yield hoverContainer("p", inspector);
  isVisible = yield isHighlighting(toolbox);
  ok(isVisible, "the highlighter is visible again");
});
