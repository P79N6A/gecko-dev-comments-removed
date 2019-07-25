










const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-own-console.html";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);
    testOpenWebConsole();
  }, true);
}

function testOpenWebConsole()
{
  openConsole();
  is(HUDService.displaysIndex().length, 1, "WebConsole was opened");

  hudId = HUDService.displaysIndex()[0];
  hud = HUDService.hudWeakReferences[hudId].get();

  testOwnConsole();
}

function testConsoleOnPage(console) {
  
  isnot(console, undefined, "Console object defined on page");
  is(console.foo, "bar", "Custom console is not overwritten");
}

function testOwnConsole()
{
  let console = browser.contentWindow.wrappedJSObject.console;
  
  
  testConsoleOnPage(console);

  
  
  ok(hud.jsterm.console, "JSTerm console is defined");
  ok(hud.jsterm.console === hud._console, "JSTerm console is same as HUD console");

  let iframe =
    browser.contentWindow.document.querySelector("iframe");

  function consoleTester()
  {
    testIFrameConsole(iframe);
  }

  iframe.contentWindow.
    addEventListener("load", consoleTester ,false);

  iframe.contentWindow.document.location = "http://example.com/";

  function testIFrameConsole(iFrame)
  {
    iFrame.contentWindow.removeEventListener("load", consoleTester, true);

    
    let consoleIFrame = iFrame.wrappedJSObject.contentWindow.console;
    
    
    

    
    HUDService.deactivateHUDForContext(tab);

    executeSoon(function () {
      consoleIFrame = iFrame.contentWindow.console;
      is(consoleIFrame, undefined, "Console object was removed from iFrame");
      testConsoleOnPage(browser.contentWindow.wrappedJSObject.console);
      finishTest();
  });
}

  browser.contentWindow.wrappedJSObject.loadIFrame();
}
