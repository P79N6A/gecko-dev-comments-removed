



"use strict";



const TEST_URI = TEST_URL_ROOT + "doc_pseudoelement.html";

add_task(function*() {
  yield addTab(TEST_URI);
  let {toolbox, inspector, view} = yield openComputedView();

  yield testTopLeft(inspector, view);
});

function* testTopLeft(inspector, view) {
  let node = yield getNodeFront("#topleft", inspector.markup);
  yield selectNode(node, inspector);
  let float = getComputedViewPropertyValue(view, "float");
  is(float, "left", "The computed view shows the correct float");

  let children = yield inspector.markup.walker.children(node);
  is (children.nodes.length, 3, "Element has correct number of children");

  let beforeElement = children.nodes[0];
  yield selectNode(beforeElement, inspector);
  let top = getComputedViewPropertyValue(view, "top");
  is(top, "0px", "The computed view shows the correct top");
  let left = getComputedViewPropertyValue(view, "left");
  is(left, "0px", "The computed view shows the correct left");

  let afterElement = children.nodes[children.nodes.length-1];
  yield selectNode(afterElement, inspector);
  top = getComputedViewPropertyValue(view, "top");
  is(top, "50%", "The computed view shows the correct top");
  left = getComputedViewPropertyValue(view, "left");
  is(left, "50%", "The computed view shows the correct left");
}

