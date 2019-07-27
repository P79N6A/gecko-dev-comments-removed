




"use strict";





const TEST_URL = "data:text/html;charset=utf8,<div>test element</div>";

add_task(function*() {
  let {inspector} = yield addTab(TEST_URL).then(openInspector);

  info("Select the test node with the browser ctx menu");
  yield selectWithBrowserMenu(inspector);
  assertNodeSelected(inspector, "div");

  info("Press arrowUp to focus <body> " +
       "(which works if the node was focused properly)");
  EventUtils.synthesizeKey("VK_UP", {});
  yield waitForChildrenUpdated(inspector);
  assertNodeSelected(inspector, "body");

  info("Select the test node with the element picker");
  yield selectWithElementPicker(inspector);
  assertNodeSelected(inspector, "div");

  info("Press arrowUp to focus <body> " +
       "(which works if the node was focused properly)");
  EventUtils.synthesizeKey("VK_UP", {});
  yield waitForChildrenUpdated(inspector);
  assertNodeSelected(inspector, "body");
});

function assertNodeSelected(inspector, tagName) {
  is(inspector.selection.nodeFront.tagName.toLowerCase(), tagName,
    `The <${tagName}> node is selected`);
}

function* selectWithBrowserMenu(inspector) {
  yield executeInContent("Test:SynthesizeMouse", {
    center: true,
    selector: "div",
    options: {type: "contextmenu", button: 2}
  });

  
  
  
  try {
    document.popupNode = getNode("div");
  } catch (e) {}

  let contentAreaContextMenu = document.querySelector("#contentAreaContextMenu");
  let contextMenu = new nsContextMenu(contentAreaContextMenu);
  yield contextMenu.inspectNode();

  contentAreaContextMenu.hidden = true;
  contentAreaContextMenu.hidePopup();
  contextMenu.hiding();

  yield inspector.once("inspector-updated");
}

function* selectWithElementPicker(inspector) {
  yield inspector.toolbox.highlighterUtils.startPicker();
  yield executeInContent("Test:SynthesizeMouse", {
    center: true,
    selector: "div",
    options: {type: "mousemove"}
  });
  executeInContent("Test:SynthesizeKey", {
    key: "VK_RETURN",
    options: {}
  }, false);
  yield inspector.once("inspector-updated");
}
