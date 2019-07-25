






































const TEST_REPLACED_API_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console-replaced-api.html";

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
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.getHeadsUpDisplay(hudId);

  let msg = (shouldWarn ? "found" : "didn't find") + " API replacement warning";
  testLogEntry(hud, "disabled", msg, false, !shouldWarn);
}
