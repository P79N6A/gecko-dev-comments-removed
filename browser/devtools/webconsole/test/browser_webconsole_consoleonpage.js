










const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-own-console.html";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testOpenWebConsole);
  }, true);
}

function testOpenWebConsole(aHud)
{
  hud = aHud;
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

  
  
  ok(hud.console, "HUD console is defined");
  finishTest();
}
