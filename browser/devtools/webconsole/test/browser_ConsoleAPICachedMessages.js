





































const TEST_URI = "data:text/html,<p>Web Console test for bug 609890";

function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab(TEST_URI);

  gBrowser.selectedBrowser.addEventListener("load", testOpenUI, true);
}

function testOpenUI()
{
  gBrowser.selectedBrowser.removeEventListener("load", testOpenUI, true);

  
  

  let console = content.wrappedJSObject.console;
  console.log("log Bazzle");
  console.info("info Bazzle");
  console.warn("warn Bazzle");
  console.error("error Bazzle");

  HUDService.activateHUDForContext(gBrowser.selectedTab);
  let hudId = HUDService.getHudIdByWindow(content);
  let hud = HUDService.getHudReferenceById(hudId);

  testLogEntry(hud.outputNode, "log Bazzle",
               "Find a console log entry from before console UI is opened",
               false, null);

  HUDService.deactivateHUDForContext(gBrowser.selectedTab);

  executeSoon(finish);
}
