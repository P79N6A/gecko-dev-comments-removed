






































const TEST_REPLACED_API_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-replaced-api.html";

function test() {
  waitForExplicitFinish();

  
  addTab("about:blank");
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);
    testOpenWebConsole(false);
    executeSoon(testWarningPresent);
  }, true);
}

function testWarningPresent() {
  
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);
    testOpenWebConsole(true);
    finishTest();
  }, true);
  browser.contentWindow.location = TEST_REPLACED_API_URI;
}

function testOpenWebConsole(shouldWarn) {
  openConsole();

  hud = HUDService.getHudByWindow(content);
  ok(hud, "WebConsole was opened");

  let msg = (shouldWarn ? "found" : "didn't find") + " API replacement warning";
  testLogEntry(hud.outputNode, "disabled", msg, false, !shouldWarn);
}
