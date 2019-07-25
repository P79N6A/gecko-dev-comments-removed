




































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole();

  let console = content.wrappedJSObject.console;
  let outputNode = HUDService.getHudByWindow(content).outputNode;

  console.log("a log message");

  let node = outputNode.querySelectorAll(".hud-msg-node");

  ok(node[0].getAttribute("id") && node[0].getAttribute != "", "we have a node id");
  closeConsole();
  finishTest();
}
