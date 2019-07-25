










const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-own-console.html";

registerCleanupFunction(function() {
  Services.prefs.clearUserPref("devtools.gcli.enable");
});

function test()
{
  Services.prefs.setBoolPref("devtools.gcli.enable", false);
  addTab(TEST_URI);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);
    testOpenWebConsole();
  }, true);
}

function testOpenWebConsole()
{
  openConsole();

  hud = HUDService.getHudByWindow(content);
  ok(hud, "WebConsole was opened");

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
  finishTest();
}
