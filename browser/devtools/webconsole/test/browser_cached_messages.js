





































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-webconsole-error-observer.html";

function test()
{
  waitForExplicitFinish();

  expectUncaughtException();

  gBrowser.selectedTab = gBrowser.addTab(TEST_URI);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    testOpenUI(true);
  }, true);
}

function testOpenUI(aTestReopen)
{
  
  

  HUDService.activateHUDForContext(gBrowser.selectedTab);
  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.getHudReferenceById(hudId);

  testLogEntry(hud.outputNode, "log Bazzle",
               "Find a console log entry from before console UI is opened",
               false, null);

  testLogEntry(hud.outputNode, "error Bazzle",
               "Find a console error entry from before console UI is opened",
               false, null);

  testLogEntry(hud.outputNode, "bazBug611032", "Found the JavaScript error");
  testLogEntry(hud.outputNode, "cssColorBug611032", "Found the CSS error");

  HUDService.deactivateHUDForContext(gBrowser.selectedTab);

  if (aTestReopen) {
    HUDService.deactivateHUDForContext(gBrowser.selectedTab);
    executeSoon(testOpenUI);
  } else {
    executeSoon(finish);
  }
}
