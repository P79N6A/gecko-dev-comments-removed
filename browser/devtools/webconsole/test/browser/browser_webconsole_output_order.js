










































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testOutputOrder, false);
}

function testOutputOrder() {
  browser.removeEventListener("DOMContentLoaded", testOutputOrder, false);

  openConsole();

  let jsterm = HUDService.getHudByWindow(content).jsterm;
  let outputNode = jsterm.outputNode;

  jsterm.clearOutput();
  jsterm.execute("console.log('foo', 'bar');");

  let nodes = outputNode.querySelectorAll(".hud-msg-node");
  is(nodes.length, 3, "3 children in output");

  let executedStringFirst =
    /console\.log\('foo', 'bar'\);/.test(nodes[0].textContent);

  let outputSecond =
    /foo bar/.test(nodes[1].textContent);

  ok(executedStringFirst && outputSecond, "executed string comes first");

  jsterm.clearOutput();
  jsterm.history.splice(0, jsterm.history.length);   

  finishTest();
}

