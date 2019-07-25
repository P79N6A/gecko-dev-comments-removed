










































const TEST_DUPLICATE_ERROR_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-duplicate-error.html";

function test() {
  addTab(TEST_DUPLICATE_ERROR_URI);
  browser.addEventListener("DOMContentLoaded", testDuplicateErrors, false);
}

function testDuplicateErrors() {
  browser.removeEventListener("DOMContentLoaded", testDuplicateErrors,
                              false);
  openConsole();

  let hudId = HUDService.displaysIndex()[0];
  HUDService.clearDisplay(hudId);

  Services.console.registerListener(consoleObserver);

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

    var display = HUDService.getDisplayByURISpec(content.location.href);
    var outputNode = display.querySelectorAll(".hud-output-node")[0];

    executeSoon(function () {
      var text = outputNode.textContent;
      var error1pos = text.indexOf("fooDuplicateError1");
      ok(error1pos > -1, "found fooDuplicateError1");
      if (error1pos > -1) {
        ok(text.indexOf("fooDuplicateError1", error1pos + 1) == -1,
          "no duplicate for fooDuplicateError1");
      }

      ok(text.indexOf("test-duplicate-error.html") > -1,
        "found test-duplicate-error.html");

      finishTest();
    });
  }
};
