










































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testNullAndUndefinedOutput,
                           false);
}

function testNullAndUndefinedOutput() {
  browser.removeEventListener("DOMContentLoaded",
                              testNullAndUndefinedOutput, false);

  openConsole();

  let jsterm = HUDService.getHudByWindow(content).jsterm;
  let outputNode = jsterm.outputNode;

  jsterm.clearOutput();
  jsterm.execute("null;");

  let nodes = outputNode.querySelectorAll(".hud-msg-node");
  is(nodes.length, 2, "2 nodes in output");
  ok(nodes[1].textContent.indexOf("null") > -1, "'null' printed to output");

  jsterm.clearOutput();
  jsterm.execute("undefined;");

  nodes = outputNode.querySelectorAll(".hud-msg-node");
  is(nodes.length, 2, "2 nodes in output");
  ok(nodes[1].textContent.indexOf("undefined") > -1, "'undefined' printed to output");

  jsterm.clearOutput();
  jsterm.history.splice(0, jsterm.history.length);   

  finishTest();
}

