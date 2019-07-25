









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded",
                              testLiveFilteringOfMessageTypes, false);
}

function testLiveFilteringOfMessageTypes() {
  browser.removeEventListener("DOMContentLoaded",
                              testLiveFilteringOfMessageTypes, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];
  let console = browser.contentWindow.wrappedJSObject.console;
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let outputNode = hudBox.querySelector(".hud-output-node");

  for (let i = 0; i < 50; i++) {
    console.log("http://www.example.com/");
  }

  HUDService.setFilterState(hudId, "log", false);
  is(countMessageNodes(), 0, "the log nodes are hidden when the " +
    "corresponding filter is switched off");

  HUDService.setFilterState(hudId, "log", true);
  isnot(countMessageNodes(), 0, "the log nodes reappear when the " +
    "corresponding filter is switched on");

  finishTest();
}

function countMessageNodes() {
  let hudId = HUDService.displaysIndex()[0];
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let outputNode = hudBox.querySelector(".hud-output-node");

  let messageNodes = outputNode.querySelectorAll(".hud-log");
  let displayedMessageNodes = 0;
  let view = outputNode.ownerDocument.defaultView;
  for (let i = 0; i < messageNodes.length; i++) {
    let computedStyle = view.getComputedStyle(messageNodes[i], null);
    if (computedStyle.display !== "none") {
      displayedMessageNodes++;
    }
  }

  return displayedMessageNodes;
}
