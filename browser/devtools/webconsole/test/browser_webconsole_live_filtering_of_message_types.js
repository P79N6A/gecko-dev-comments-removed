






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

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
    console.log("http://www.example.com/" + i);
  }

  waitForSuccess({
    name: "50 console.log messages displayed",
    validatorFn: function()
    {
      return hud.outputNode.itemCount == 50;
    },
    successFn: testLiveFilteringOfMessageTypes,
    failureFn: finishTest,
  });
}

function testLiveFilteringOfMessageTypes() {
  hud.setFilterState("log", false);
  is(countMessageNodes(), 0, "the log nodes are hidden when the " +
    "corresponding filter is switched off");

  hud.setFilterState("log", true);
  is(countMessageNodes(), 50, "the log nodes reappear when the " +
    "corresponding filter is switched on");

  finishTest();
}

function countMessageNodes() {
  let messageNodes = hud.outputNode.querySelectorAll(".hud-log");
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
