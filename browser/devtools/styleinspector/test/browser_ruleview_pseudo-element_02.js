



"use strict";



const TEST_URI = TEST_URL_ROOT + "doc_pseudoelement.html";

add_task(function*() {
  yield addTab(TEST_URI);
  let {toolbox, inspector, view} = yield openRuleView();

  yield testTopLeft(inspector, view);
});

function* testTopLeft(inspector, view) {
  let node = inspector.markup.walker.frontForRawNode(getNode("#topleft"));
  let children = yield inspector.markup.walker.children(node);

  is (children.nodes.length, 3, "Element has correct number of children");

  let beforeElement = children.nodes[0];
  is (beforeElement.tagName, "_moz_generated_content_before", "tag name is correct");
  yield selectNode(beforeElement, inspector);

  let afterElement = children.nodes[children.nodes.length-1];
  is (afterElement.tagName, "_moz_generated_content_after", "tag name is correct");
  yield selectNode(afterElement, inspector);
}

