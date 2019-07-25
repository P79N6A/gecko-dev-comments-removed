










































const TEST_NETWORK_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-network.html" + "?_date=" + Date.now();

function test() {
  addTab("data:text/html,Web Console basic network logging test");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);
  openConsole();
  hudId = HUDService.displaysIndex()[0];

  browser.addEventListener("load", testBasicNetLogging, true);
  content.location = TEST_NETWORK_URI;
}

function testBasicNetLogging(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  hudBox = HUDService.getHeadsUpDisplay(hudId);

  executeSoon(function() {
    let text = hudBox.querySelector(".hud-output-node").textContent;

    isnot(text.indexOf("test-network.html"), -1, "found test-network.html");
    isnot(text.indexOf("testscript.js"), -1, "found testscript.js");
    isnot(text.indexOf("test-image.png"), -1, "found test-image.png");
    isnot(text.indexOf("network console"), -1, "found network console");

    finishTest();
  });
}

