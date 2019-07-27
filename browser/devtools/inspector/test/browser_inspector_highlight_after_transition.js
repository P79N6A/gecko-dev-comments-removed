



"use strict";

const TEST_URI = TEST_URL_ROOT + "doc_inspector_highlight_after_transition.html";



add_task(function*() {
  info("Loading the test document and opening the inspector");

  let {inspector} = yield openInspectorForURL(TEST_URI);

  yield checkDivHeight(inspector);
});

function* checkDivHeight(inspector) {
  let div = getNode("div");

  div.setAttribute("visible", "true");

  yield once(div, "transitionend");
  yield selectAndHighlightNode(div, inspector);

  let height = div.getBoundingClientRect().height;

  is (height, 201, "div is the correct height");
}
