





"use strict";



const TEST_URI = TEST_URL_ROOT + "doc_inspector_highlighter.html";

add_task(function*() {
  let {toolbox, inspector} = yield openInspectorForURL(TEST_URI);

  info("Selecting the simple, non-transformed DIV");
  yield selectAndHighlightNode("#simple-div", inspector);

  let isVisible = yield isHighlighting(toolbox);
  ok(isVisible, "The highlighter is shown");
  let highlightedNode = yield getHighlitNode(toolbox);
  is(highlightedNode, getNode("#simple-div"),
    "The highlighter's outline corresponds to the simple div");
  yield isNodeCorrectlyHighlighted("#simple-div", toolbox,
    "non-zoomed");

  info("Selecting the rotated DIV");
  yield selectAndHighlightNode("#rotated-div", inspector);

  isVisible = yield isHighlighting(toolbox);
  ok(isVisible, "The highlighter is shown");
  yield isNodeCorrectlyHighlighted("#rotated-div", toolbox,
    "rotated");

  info("Selecting the zero width height DIV");
  yield selectAndHighlightNode("#widthHeightZero-div", inspector);

  isVisible = yield isHighlighting(toolbox);
  ok(isVisible, "The highlighter is shown");
  yield isNodeCorrectlyHighlighted("#widthHeightZero-div", toolbox,
    "zero width height");
});
