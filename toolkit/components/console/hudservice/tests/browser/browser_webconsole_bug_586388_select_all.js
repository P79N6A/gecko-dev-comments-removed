









const TEST_URI = "http://example.com/";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded",
                           testSelectionWhenMovingBetweenBoxes, false);
}

function testSelectionWhenMovingBetweenBoxes() {
  browser.removeEventListener("DOMContentLoaded",
                              testSelectionWhenMovingBetweenBoxes, false);
  openConsole();

  let hudId = HUDService.displaysIndex()[0];
  let jsterm = HUDService.hudReferences[hudId].jsterm;

  
  jsterm.clearOutput();
  jsterm.execute("1 + 2");
  jsterm.execute("3 + 4");
  jsterm.execute("5 + 6");

  outputNode = jsterm.outputNode;

  ok(outputNode.childNodes.length >= 3, "the output node has children after " +
     "executing some JavaScript");

  
  
  let commandController = window.commandController;
  ok(commandController != null, "the window has a command controller object");

  commandController.selectAll(outputNode);
  is(outputNode.selectedCount, outputNode.childNodes.length, "all console " +
     "messages are selected after performing a regular browser select-all " +
     "operation");

  outputNode.selectedIndex = -1;

  
  
  let contextMenuId = outputNode.getAttribute("context");
  let contextMenu = document.getElementById(contextMenuId);
  ok(contextMenu != null, "the output node has a context menu");

  let selectAllItem = contextMenu.querySelector("*[buttonType=\"selectAll\"]");
  ok(selectAllItem != null,
     "the context menu on the output node has a \"Select All\" item");

  let commandEvent = document.createEvent("XULCommandEvent");
  commandEvent.initCommandEvent("command", true, true, window, 0, false, false,
                                false, false, null);
  selectAllItem.dispatchEvent(commandEvent);

  is(outputNode.selectedCount, outputNode.childNodes.length, "all console " +
     "messages are selected after performing a select-all operation from " +
     "the context menu");

  outputNode.selectedIndex = -1;

  commandEvent = contextMenu = groupNode = range = null;

  finishTest();
}

