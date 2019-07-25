










































const TEST_DUPLICATE_ERROR_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-duplicate-error.html";

function test() {
  expectUncaughtException();
  addTab(TEST_DUPLICATE_ERROR_URI);
  browser.addEventListener("DOMContentLoaded", testDuplicateErrors, false);
}

function testDuplicateErrors() {
  browser.removeEventListener("DOMContentLoaded", testDuplicateErrors,
                              false);
  openConsole();

  HUDService.getHudByWindow(content).jsterm.clearOutput();

  Services.console.registerListener(consoleObserver);

  expectUncaughtException();
  content.location.reload();
}

var consoleObserver = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  observe: function (aMessage)
  {
    
    if (!(aMessage instanceof Ci.nsIScriptError) ||
      aMessage.category != "content javascript") {
      return;
    }

    Services.console.unregisterListener(this);

    outputNode = HUDService.getHudByWindow(content).outputNode;

    executeSoon(function () {
      var text = outputNode.textContent;
      var error1pos = text.indexOf("fooDuplicateError1");
      ok(error1pos > -1, "found fooDuplicateError1");
      if (error1pos > -1) {
        ok(text.indexOf("fooDuplicateError1", error1pos + 1) == -1,
          "no duplicate for fooDuplicateError1");
      }

      findLogEntry("test-duplicate-error.html");

      finishTest();
    });
  }
};
