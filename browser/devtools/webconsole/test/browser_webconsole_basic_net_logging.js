










































const TEST_NETWORK_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-network.html" + "?_date=" + Date.now();

function test() {
  addTab("data:text/html,Web Console basic network logging test");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);
  openConsole();

  browser.addEventListener("load", testBasicNetLogging, true);
  content.location = TEST_NETWORK_URI;
}

function testBasicNetLogging(aEvent) {
  browser.removeEventListener(aEvent.type, arguments.callee, true);

  outputNode = HUDService.getHudByWindow(content).outputNode;

  executeSoon(function() {
    findLogEntry("test-network.html");
    findLogEntry("testscript.js");
    findLogEntry("test-image.png");
    findLogEntry("network console");

    finishTest();
  });
}

