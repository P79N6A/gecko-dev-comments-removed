









































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
  jsterm.execute("location;");

  let nodes = jsterm.outputNode.querySelectorAll(".hud-msg-node");
  is(nodes.length, 2, "Two children in output");

  is(/location;/.test(nodes[0].textContent), true,
     "'location;' written to output");

  ok(nodes[0].textContent.indexOf(TEST_URI),
    "command was executed in the window scope");

  executeSoon(finishTest);
}

