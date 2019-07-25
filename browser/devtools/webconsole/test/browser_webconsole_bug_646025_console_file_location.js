







const TEST_URI = "http://example.com/browser/browser/devtools/" +
                 "webconsole/test/" +
                 "test-bug-646025-console-file-location.html";

function test() {
  addTab("data:text/html;charset=utf-8,Web Console file location display test");
  browser.addEventListener("load", onLoad, true);
}

function onLoad(aEvent) {
  browser.removeEventListener(aEvent.type, onLoad, true);
  openConsole(null, function(aHud) {
    hud = aHud;
    browser.addEventListener("load", testConsoleFileLocation, true);
    content.location = TEST_URI;
  });
}

function testConsoleFileLocation(aEvent) {
  browser.removeEventListener(aEvent.type, testConsoleFileLocation, true);

  outputNode = hud.outputNode;

  waitForSuccess({
    name: "console API messages",
    validatorFn: function()
    {
      return outputNode.textContent.indexOf("message for level debug") > -1;
    },
    successFn: function()
    {
      findLogEntry("test-file-location.js");
      findLogEntry("message for level");
      findLogEntry("test-file-location.js:5");
      findLogEntry("test-file-location.js:6");
      findLogEntry("test-file-location.js:7");
      findLogEntry("test-file-location.js:8");
      findLogEntry("test-file-location.js:9");

      finishTest();
    },
    failureFn: finishTest,
  });
}

