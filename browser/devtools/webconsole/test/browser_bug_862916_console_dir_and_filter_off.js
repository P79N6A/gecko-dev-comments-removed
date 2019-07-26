






const TEST_URI = "data:text/html;charset=utf8,<p>test for bug 862916";

function test()
{
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(hud)
{
  ok(hud, "web console opened");

  hud.setFilterState("log", false);
  registerCleanupFunction(() => hud.setFilterState("log", true));

  content.wrappedJSObject.fooBarz = "bug862916";
  hud.jsterm.execute("console.dir(window)");
  hud.jsterm.once("variablesview-fetched", (aEvent, aVar) => {
    ok(aVar, "variables view object");
    findVariableViewProperties(aVar, [
      { name: "fooBarz", value: "bug862916" },
    ], { webconsole: hud }).then(finishTest);
  });
}
