



"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/inspector/" +
                 "test/browser_inspector_highlight_after_transition.html";



add_task(function*() {
  info("Loading the test document and opening the inspector");

  yield addTab(TEST_URI);

  let {inspector} = yield openInspector();

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
