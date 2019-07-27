


"use strict";



const TEST_URL = TEST_URL_ROOT + "doc_inspector_delete-selected-node-01.html";

add_task(function* () {
  let {inspector} = yield openInspectorForURL(TEST_URL);

  let span = yield getNodeFrontInFrame("span", "iframe", inspector);
  yield selectNode(span, inspector);

  info("Removing selected <span> element.");
  let parentNode = span.parentNode();
  yield inspector.walker.removeNode(span);

  
  yield inspector.once("inspector-updated");
  is(inspector.selection.nodeFront, parentNode,
    "Parent node of selected <span> got selected.");
});
