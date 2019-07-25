




































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole(null, function(hud) {
    content.console.log("a log message");

    waitForSuccess({
      name: "console.log message shown with an ID attribute",
      validatorFn: function()
      {
        let node = hud.outputNode.querySelector(".hud-msg-node");
        return node && node.getAttribute("id");
      },
      successFn: finishTest,
      failureFn: finishTest,
    });
  });
}
