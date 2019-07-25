









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testGroups, false);
}

function testGroups() {
  browser.removeEventListener("DOMContentLoaded", testGroups, false);

  openConsole();

  let hudId = HUDService.displaysIndex()[0];

  let HUD = HUDService.hudWeakReferences[hudId].get();
  let jsterm = HUD.jsterm;
  let outputNode = jsterm.outputNode;

  let timestamp0 = Date.now();
  jsterm.execute("0");
  is(outputNode.querySelectorAll(".hud-group").length, 1,
    "one group exists after the first console message");

  jsterm.execute("1");
  let timestamp1 = Date.now();
  if (timestamp1 - timestamp0 < 5000) {
    is(outputNode.querySelectorAll(".hud-group").length, 1,
      "only one group still exists after the second console message");
  }

  HUD.HUDBox.lastTimestamp = 0;   
  jsterm.execute("2");
  is(outputNode.querySelectorAll(".hud-group").length, 2,
    "two groups exist after the third console message");

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  finishTest();
}

