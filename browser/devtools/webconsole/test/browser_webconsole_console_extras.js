






































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-extras.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  let doc = content.document;
  openConsole();
  let button = doc.querySelector("button");
  ok(button, "we have the button");
  EventUtils.sendMouseEvent({ type: "click" }, button, content);
  executeSoon(testButtonClicked);
}

function testButtonClicked()
{
  let outputNode = HUDService.getHudByWindow(content).outputNode;
  let nodes = outputNode.querySelectorAll(".hud-msg-node");
  is(nodes.length, 2, "two nodes");
  ok(/start/.test(nodes[0].textContent), "start found");
  ok(/end/.test(nodes[1].textContent), "end found - complete!");
  finishTest();
}
