






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

let hud;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(aHud) {
  hud = aHud;
  hud.jsterm.clearOutput();

  let console = content.console;

  for (let i = 0; i < 50; i++) {
    console.log("foobarz #" + i);
  }

  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "foobarz #49",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(testLiveFilteringOfMessageTypes);
}

function testLiveFilteringOfMessageTypes() {
  is(hud.outputNode.children.length, 50, "number of messages");

  hud.setFilterState("log", false);
  is(countMessageNodes(), 0, "the log nodes are hidden when the " +
    "corresponding filter is switched off");

  hud.setFilterState("log", true);
  is(countMessageNodes(), 50, "the log nodes reappear when the " +
    "corresponding filter is switched on");

  finishTest();
}

function countMessageNodes() {
  let messageNodes = hud.outputNode.querySelectorAll(".message");
  let displayedMessageNodes = 0;
  let view = hud.iframeWindow;
  for (let i = 0; i < messageNodes.length; i++) {
    let computedStyle = view.getComputedStyle(messageNodes[i], null);
    if (computedStyle.display !== "none") {
      displayedMessageNodes++;
    }
  }

  return displayedMessageNodes;
}
