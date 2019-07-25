









const Cu = Components.utils;

Cu.import("resource://gre/modules/HUDService.jsm");

const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  waitForExplicitFinish();
  content.location.href = TEST_URI;
  waitForFocus(onFocus);
}

function onFocus() {
  let tabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  tabBrowser.addEventListener("DOMContentLoaded",
                              testSelectionWhenMovingBetweenBoxes, false);
}

function testSelectionWhenMovingBetweenBoxes() {
  let tabBrowser = gBrowser.getBrowserForTab(gBrowser.selectedTab);
  tabBrowser.removeEventListener("DOMContentLoaded",
                                 testSelectionWhenMovingBetweenBoxes, false);

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

  let selectAllItem = contextMenu.childNodes[1];
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
  finish();
}

