




































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", onLoad, false);
}

function onLoad() {
  browser.removeEventListener("DOMContentLoaded", onLoad, false);
  openConsole();

  let hudId = HUDService.displaysIndex()[0];
  let console = browser.contentWindow.wrappedJSObject.console;
  let hudBox = HUDService.getHeadsUpDisplay(hudId);
  let outputNode = hudBox.querySelector(".hud-output-node");

  console.log("a log message");

  let node = outputNode.querySelectorAll(".hud-msg-node");

  ok(node[0].getAttribute("id") && node[0].getAttribute != "", "we have a node id");
  closeConsole();
  finishTest();
}
