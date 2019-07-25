









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded",
                              testLiveFilteringOfMessageTypes, false);
}

function testLiveFilteringOfMessageTypes() {
  browser.removeEventListener("DOMContentLoaded",
                              testLiveFilteringOfMessageTypes, false);

  openConsole();

  hud = HUDService.getHudByWindow(content);
  let console = browser.contentWindow.wrappedJSObject.console;

  for (let i = 0; i < 50; i++) {
    console.log("http://www.example.com/");
  }

  HUDService.setFilterState(hud.hudId, "log", false);
  is(countMessageNodes(), 0, "the log nodes are hidden when the " +
    "corresponding filter is switched off");

  HUDService.setFilterState(hud.hudId, "log", true);
  isnot(countMessageNodes(), 0, "the log nodes reappear when the " +
    "corresponding filter is switched on");

  finishTest();
}

function countMessageNodes() {
  let messageNodes = hud.outputNode.querySelectorAll(".hud-log");
  let displayedMessageNodes = 0;
  let view = hud.chromeWindow;
  for (let i = 0; i < messageNodes.length; i++) {
    let computedStyle = view.getComputedStyle(messageNodes[i], null);
    if (computedStyle.display !== "none") {
      displayedMessageNodes++;
    }
  }

  return displayedMessageNodes;
}
