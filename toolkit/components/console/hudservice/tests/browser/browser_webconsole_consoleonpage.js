










const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource:///modules/HUDService.jsm");

let hud;
let hudId;

function testOpenWebConsole()
{
  HUDService.activateHUDForContext(gBrowser.selectedTab);
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.hudWeakReferences[hudId].get();

  testOwnConsole();
}

function testConsoleOnPage() {
  let console = content.wrappedJSObject.console;
  isnot(console, undefined, "Console object defined on page");
  is(console.foo, "bar", "Custom console is not overwritten");
}

function testOwnConsole()
{
  
  
  testConsoleOnPage();

  
  
  ok(hud.jsterm.console, "JSTerm console is defined");
  ok(hud.jsterm.console === hud._console, "JSTerm console is same as HUD console");

  content.wrappedJSObject.loadIFrame(function(iFrame) {
    
    let consoleIFrame = iFrame.wrappedJSObject.contentWindow.console;
    isnot(consoleIFrame, undefined, "Console object defined in iFrame");

    ok(consoleIFrame === hud._console, "Console on the page is hud console");

    
    HUDService.deactivateHUDForContext(gBrowser.selectedTab);

    executeSoon(function () {
      consoleIFrame = iFrame.wrappedJSObject.contentWindow.console;
      is(consoleIFrame, undefined, "Console object was removed from iFrame");
      testConsoleOnPage();

      hud = hudId = null;
      gBrowser.removeCurrentTab();
      finish();
    });
  });
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    waitForFocus(testOpenWebConsole, content);
  }, true);

  content.location =
    "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-own-console.html";
}
