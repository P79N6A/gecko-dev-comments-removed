


"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_inspector_delete-selected-node-01.html";

add_task(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URL);

  let iframe = yield getNodeFront("iframe", inspector);
  let node = yield getNodeFrontInFrame("span", iframe, inspector);
  yield selectNode(node, inspector);

  info("Removing iframe.");
  yield inspector.walker.removeNode(iframe);
  yield inspector.selection.once("detached-front");

  let body = yield getNodeFront("body", inspector);

  is(inspector.selection.nodeFront, body, "Selection is now the body node");

  yield inspector.once("inspector-updated");
});
