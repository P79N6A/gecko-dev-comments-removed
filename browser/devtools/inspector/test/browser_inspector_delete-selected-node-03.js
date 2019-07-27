


"use strict";




const TEST_URL = TEST_URL_ROOT + "doc_inspector_delete-selected-node-01.html";

let test = asyncTest(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URL);

  let iframe = getNode("iframe");
  let node = getNode("span", { document: iframe.contentDocument });
  yield selectNode(node, inspector);

  info("Removing iframe.");
  iframe.remove();

  let lh = new LayoutHelpers(window.content);
  ok(!lh.isNodeConnected(node), "Node considered as disconnected.");
  ok(!inspector.selection.isConnected(), "Selection considered as disconnected.");

  yield inspector.once("inspector-updated");
});
