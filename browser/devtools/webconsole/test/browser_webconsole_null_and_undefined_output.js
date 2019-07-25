







const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testNullAndUndefinedOutput);
  }, true);
}

function testNullAndUndefinedOutput(hud) {
  let jsterm = hud.jsterm;
  let outputNode = jsterm.outputNode;

  jsterm.clearOutput();
  jsterm.execute("null;");

  waitForSuccess({
    name: "null displayed",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".hud-msg-node").length == 2;
    },
    successFn: function()
    {
      let nodes = outputNode.querySelectorAll(".hud-msg-node");
      isnot(nodes[1].textContent.indexOf("null"), -1,
            "'null' printed to output");

      jsterm.clearOutput();
      jsterm.execute("undefined;");
      waitForSuccess(waitForUndefined);
    },
    failureFn: finishTest,
  });

  let waitForUndefined = {
    name: "undefined displayed",
    validatorFn: function()
    {
      return outputNode.querySelectorAll(".hud-msg-node").length == 2;
    },
    successFn: function()
    {
      let nodes = outputNode.querySelectorAll(".hud-msg-node");
      isnot(nodes[1].textContent.indexOf("undefined"), -1,
            "'undefined' printed to output");

      finishTest();
    },
    failureFn: finishTest,
  };
}

