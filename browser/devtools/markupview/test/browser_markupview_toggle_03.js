



"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_markup_toggle.html";

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Getting the container for the UL parent element");
  let container = getContainerForRawNode("ul", inspector);

  info("Alt-clicking on the UL parent expander, and waiting for children");
  let onUpdated = inspector.once("inspector-updated");
  EventUtils.synthesizeMouseAtCenter(container.expander, {altKey: true},
    inspector.markup.doc.defaultView);
  yield onUpdated;
  yield waitForMultipleChildrenUpdates(inspector);

  info("Checking that all nodes exist and are expanded");
  for (let node of content.document.querySelectorAll("ul, li, span, em")) {
    let nodeContainer = getContainerForRawNode(node, inspector);
    ok(nodeContainer, "Container for node " + node.tagName + " exists");
    ok(nodeContainer.expanded,
      "Container for node " + node.tagName + " is expanded");
  }
});



function* waitForMultipleChildrenUpdates(inspector) {
  
  
  if (inspector.markup._queuedChildUpdates &&
      inspector.markup._queuedChildUpdates.size) {
    yield waitForChildrenUpdated(inspector);
    return yield waitForMultipleChildrenUpdates(inspector);
  }
}