







const TEST_NETWORK_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-network.html" + "?_date=" + Date.now();

function test() {
  addTab("data:text/html;charset=utf-8,Web Console basic network logging test");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, onLoad, true);
  openConsole(null, function() {
    browser.addEventListener("load", testBasicNetLogging, true);
    content.location = TEST_NETWORK_URI;
  });
}

function testBasicNetLogging(aEvent) {
  browser.removeEventListener(aEvent.type, testBasicNetLogging, true);

  outputNode = HUDService.getHudByWindow(content).outputNode;

  waitForSuccess({
    name: "network console message",
    validatorFn: function()
    {
      return outputNode.textContent.indexOf("running network console") > -1;
    },
    successFn: function()
    {
      findLogEntry("test-network.html");
      findLogEntry("testscript.js");
      findLogEntry("test-image.png");
      finishTest();
    },
    failureFn: finishTest,
  });
}

