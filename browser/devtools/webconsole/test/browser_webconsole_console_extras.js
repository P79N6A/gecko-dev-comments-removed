






































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-extras.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(consoleOpened);
  }, true);
}

function consoleOpened(hud) {
  waitForSuccess({
    name: "two nodes displayed",
    validatorFn: function()
    {
      return hud.outputNode.querySelectorAll(".hud-msg-node").length == 2;
    },
    successFn: function()
    {
      let nodes = hud.outputNode.querySelectorAll(".hud-msg-node");
      ok(/start/.test(nodes[0].textContent), "start found");
      ok(/end/.test(nodes[1].textContent), "end found - complete!");

      finishTest();
    },
    failureFn: finishTest,
  });

  let button = content.document.querySelector("button");
  ok(button, "we have the button");
  EventUtils.sendMouseEvent({ type: "click" }, button, content);
}
