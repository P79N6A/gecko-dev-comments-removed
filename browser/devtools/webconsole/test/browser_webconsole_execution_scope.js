






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testExecutionScope);
  }, true);
}

function testExecutionScope(hud) {
  let jsterm = hud.jsterm;

  jsterm.clearOutput();
  jsterm.execute("window.location.href;");

  waitForSuccess({
    name: "jsterm execution output (two nodes)",
    validatorFn: function()
    {
      return jsterm.outputNode.querySelectorAll(".hud-msg-node").length == 2;
    },
    successFn: function()
    {
      let nodes = jsterm.outputNode.querySelectorAll(".hud-msg-node");

      is(/window.location.href;/.test(nodes[0].textContent), true,
        "'window.location.href;' written to output");

      isnot(nodes[1].textContent.indexOf(TEST_URI), -1,
        "command was executed in the window scope");

      executeSoon(finishTest);
    },
    failureFn: finishTest,
  });
}

