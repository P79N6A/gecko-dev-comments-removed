









































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

registerCleanupFunction(function() {
  Services.prefs.clearUserPref("devtools.gcli.enable");
});

function test() {
  Services.prefs.setBoolPref("devtools.gcli.enable", false);
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testExecutionScope, false);
}

function testExecutionScope() {
  browser.removeEventListener("DOMContentLoaded", testExecutionScope,
                              false);

  openConsole();

  let jsterm = HUDService.getHudByWindow(content).jsterm;

  jsterm.clearOutput();
  jsterm.execute("location;");

  let nodes = jsterm.outputNode.querySelectorAll(".hud-msg-node");
  is(nodes.length, 2, "Two children in output");

  is(/location;/.test(nodes[0].textContent), true,
     "'location;' written to output");

  ok(nodes[0].textContent.indexOf(TEST_URI),
    "command was executed in the window scope");

  jsterm.clearOutput();
  jsterm.history.splice(0, jsterm.history.length);   

  finishTest();
}

