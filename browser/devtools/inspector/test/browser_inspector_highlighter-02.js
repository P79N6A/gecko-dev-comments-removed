





"use strict";



const TEST_URI = TEST_URL_ROOT + "doc_inspector_highlighter.html";

add_task(function*() {
  let {toolbox, inspector, testActor} = yield openInspectorForURL(TEST_URI);

  info("Selecting the simple, non-transformed DIV");
  yield selectAndHighlightNode("#simple-div", inspector);

  let isVisible = yield testActor.isHighlighting();
  ok(isVisible, "The highlighter is shown");
  ok((yield testActor.assertHighlightedNode("#simple-div")),
    "The highlighter's outline corresponds to the simple div");
  yield testActor.isNodeCorrectlyHighlighted("#simple-div", is, "non-zoomed");

  info("Selecting the rotated DIV");
  yield selectAndHighlightNode("#rotated-div", inspector);

  isVisible = yield testActor.isHighlighting();
  ok(isVisible, "The highlighter is shown");
  yield testActor.isNodeCorrectlyHighlighted("#rotated-div", is, "rotated");

  info("Selecting the zero width height DIV");
  yield selectAndHighlightNode("#widthHeightZero-div", inspector);

  isVisible = yield testActor.isHighlighting();
  ok(isVisible, "The highlighter is shown");
  yield testActor.isNodeCorrectlyHighlighted("#widthHeightZero-div", is, "zero width height");
});
