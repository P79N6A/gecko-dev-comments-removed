










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testNullAndUndefinedOutput,
                           false);
}

function testNullAndUndefinedOutput() {
  browser.removeEventListener("DOMContentLoaded",
                              testNullAndUndefinedOutput, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  let HUD = HUDService.hudWeakReferences[hudId].get();
  let jsterm = HUD.jsterm;
  outputNode = jsterm.outputNode;

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
  jsterm.history.splice(0);   

  finishTest();
}

