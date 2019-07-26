








const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-extras.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(hud) {
  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "start",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    },
    {
      text: "end",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(() => {
    let nodes = hud.outputNode.querySelectorAll(".message");
    is(nodes.length, 2, "only two messages are displayed");
    finishTest();
  });

  let button = content.document.querySelector("button");
  ok(button, "we have the button");
  EventUtils.sendMouseEvent({ type: "click" }, button, content);
}
