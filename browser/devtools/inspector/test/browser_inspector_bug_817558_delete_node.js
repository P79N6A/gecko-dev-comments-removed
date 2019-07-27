


"use strict";



const TEST_URL = TEST_URL_ROOT + "browser_inspector_destroyselection.html";

let test = asyncTest(function* () {
  let { inspector } = yield openInspectorForURL(TEST_URL);
  let iframe = getNode("iframe");
  let span = getNode("span", { document: iframe.contentDocument });

  yield selectNode(span, inspector);

  info("Removing selected <span> element.");
  let parentNode = span.parentNode;
  span.remove();

  let lh = new LayoutHelpers(window.content);
  ok(!lh.isNodeConnected(span), "Node considered as disconnected.");

  
  yield inspector.once("inspector-updated");
  is(inspector.selection.node, parentNode,
    "Parent node of selected <span> got selected.");
});
