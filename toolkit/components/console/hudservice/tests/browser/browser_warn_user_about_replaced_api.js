






































const TEST_REPLACED_API_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console-replaced-api.html";

function test()
{
  addTab(TEST_REPLACED_API_URI);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee,
                                true);
    testOpenWebConsole();
  }, true);
}

function testOpenWebConsole()
{
  openConsole();
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.getHeadsUpDisplay(hudId);

  testWarning();
}

function testWarning()
{
  const successMsg = "Found the warning message";
  const errMsg = "Could not find the warning message about the replaced API";

  testLogEntry(hud, "disabled",
               { success: "Found disabled console error message",
                 err: "disable msg not found"});

  finishTest();
}
