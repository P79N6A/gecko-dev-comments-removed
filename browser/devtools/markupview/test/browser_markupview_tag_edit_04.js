



"use strict";





const TEST_URL = "data:text/html,<div id='parent'><div id='first'></div><div id='second'></div><div id='third'></div></div>";

function* checkDeleteAndSelection(inspector, nodeSelector, focusedNodeSelector) {
  yield selectNode(nodeSelector, inspector);
  yield clickContainer(nodeSelector, inspector);

  info("Deleting the element \"" + nodeSelector + "\" with the keyboard");
  let mutated = inspector.once("markupmutation");
  EventUtils.sendKey("delete", inspector.panelWin);

  yield Promise.all([mutated, inspector.once("inspector-updated")]);

  let nodeFront = yield getNodeFront(focusedNodeSelector, inspector);
  is(inspector.selection.nodeFront, nodeFront,
    focusedNodeSelector + " should be selected after " + nodeSelector + " node gets deleted.");

  info("Checking that it's gone, baby gone!");
  ok(!content.document.querySelector(nodeSelector), "The test node does not exist");

  yield undoChange(inspector);
  ok(content.document.querySelector(nodeSelector), "The test node is back!");
}

let test = asyncTest(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Selecting the test node by clicking on it to make sure it receives focus");

  yield checkDeleteAndSelection(inspector, "#first", "#parent");
  yield checkDeleteAndSelection(inspector, "#second", "#first");
  yield checkDeleteAndSelection(inspector, "#third", "#second");

  yield inspector.once("inspector-updated");
});
