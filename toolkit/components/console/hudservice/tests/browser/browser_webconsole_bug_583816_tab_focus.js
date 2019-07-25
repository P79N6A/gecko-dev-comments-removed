









const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

let inputNode;

function tabLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  waitForFocus(function() {
    openConsole();

    let hudId = HUDService.getHudIdByWindow(content);
    HUD = HUDService.hudReferences[hudId];

    let display = HUDService.getOutputNodeById(hudId);
    inputNode = display.querySelector(".jsterm-input-node");

    inputNode.focus();
    executeSoon(function() {
      is(inputNode.getAttribute("focused"), "true", "inputNode is focused");
      HUD.jsterm.setInputValue("doc");
      inputNode.addEventListener("keyup", firstTab, false);
      EventUtils.synthesizeKey("VK_TAB", {});
    });
  }, content);
}

function firstTab(aEvent) {
  this.removeEventListener(aEvent.type, arguments.callee, false);

  is(inputNode.getAttribute("focused"), "true", "inputNode is still focused");
  isnot(this.value, "doc", "input autocompleted");

  HUD.jsterm.setInputValue("foobarbaz" + Math.floor(Date.now()));

  EventUtils.synthesizeKey("VK_TAB", {});

  executeSoon(secondTab);
}

function secondTab() {
  isnot(inputNode.getAttribute("focused"), "true",
          "inputNode is no longer focused");

  HUD = inputNode = null;
  HUDService.deactivateHUDForContext(gBrowser.selectedTab);
  executeSoon(finish);
}

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", tabLoad, true);
}

