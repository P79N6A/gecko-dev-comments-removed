



"use strict";



const TEST_URL = "data:text/html,<div id='delete-me'></div>";

let test = asyncTest(function*() {
  let {toolbox, inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Selecting the test node by clicking on it to make sure it receives focus");
  let node = content.document.querySelector("#delete-me");
  yield clickContainer("#delete-me", inspector);

  info("Deleting the element with the keyboard");
  let mutated = inspector.once("markupmutation");
  EventUtils.sendKey("delete", inspector.panelWin);
  yield mutated;

  info("Checking that it's gone, baby gone!");
  ok(!content.document.querySelector("#delete-me"), "The test node does not exist");

  yield undoChange(inspector);
  ok(content.document.querySelector("#delete-me"), "The test node is back!");

  yield inspector.once("inspector-updated");
});
