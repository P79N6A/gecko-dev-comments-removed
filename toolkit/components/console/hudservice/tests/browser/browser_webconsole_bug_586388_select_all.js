









const Cu = Components.utils;

Cu.import("resource://gre/modules/HUDService.jsm");

const TEST_URI = "http://example.com/";

function test() {
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab(TEST_URI);
  gBrowser.selectedBrowser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  gBrowser.selectedBrowser.removeEventListener("DOMContentLoaded", onLoad,
                                               false);
  executeSoon(testSelectionWhenMovingBetweenBoxes);
}

function testSelectionWhenMovingBetweenBoxes() {
  HUDService.activateHUDForContext(gBrowser.selectedTab);

  let hudId = HUDService.displaysIndex()[0];
  let jsterm = HUDService.hudWeakReferences[hudId].get().jsterm;

  
  jsterm.clearOutput();
  jsterm.execute("1 + 2");
  jsterm.execute("3 + 4");
  jsterm.execute("5 + 6");

  let outputNode = jsterm.outputNode;
  let groupNode = outputNode.querySelector(".hud-group");

  ok(groupNode.childNodes.length >= 3, "the output node has children after " +
     "executing some JavaScript");

  
  
  let selection = window.getSelection();
  selection.removeAllRanges();
  let range = document.createRange();
  range.selectNode(outputNode.firstChild);
  selection.addRange(range);
  selection.collapseToStart();

  let commandController = window.commandController;
  ok(commandController != null, "the window has a command controller object");

  commandController.selectAll(outputNode);
  for (let i = 0; i < groupNode.childNodes.length; i++) {
    ok(selection.containsNode(groupNode.childNodes[i], false),
       "HUD message " + i + " is selected after performing a regular " +
       "browser select-all operation");
  }

  selection.removeAllRanges();

  
  
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

  for (let i = 0; i < groupNode.childNodes.length; i++) {
    ok(selection.containsNode(groupNode.childNodes[i], false),
       "HUD message " + i + " is selected after performing a select-all " +
       "operation from the context menu");
  }

  selection.removeAllRanges();

  HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  gBrowser.removeCurrentTab();
  finish();
}

