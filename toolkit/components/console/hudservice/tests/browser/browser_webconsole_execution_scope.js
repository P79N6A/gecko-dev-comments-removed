









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testExecutionScope, false);
}

function testExecutionScope() {
  browser.removeEventListener("DOMContentLoaded", testExecutionScope,
                              false);

  openConsole();

  let hudId = HUDService.displaysIndex()[0];

  let HUD = HUDService.hudWeakReferences[hudId].get();
  let jsterm = HUD.jsterm;

  jsterm.clearOutput();
  jsterm.execute("location;");

  let nodes = jsterm.outputNode.querySelectorAll(".hud-msg-node");
  log(nodes[0].textContent);
  is(nodes.length, 1, "Three children in output");

  is(/location;/.test(nodes[0].textContent), true,
     "'location;' written to output");

  ok(nodes[0].textContent.indexOf(TEST_URI),
    "command was executed in the window scope");

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  finishTest();
}

