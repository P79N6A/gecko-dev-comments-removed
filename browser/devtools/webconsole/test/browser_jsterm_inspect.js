






function test()
{
  const TEST_URI = "data:text/html;charset=utf8,<p>hello bug 869981";

  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);

  function consoleOpened(hud)
  {
    content.wrappedJSObject.testProp = "testValue";

    hud.jsterm.once("variablesview-fetched", onObjFetch);
    hud.jsterm.execute("inspect(window)");
  }

  function onObjFetch(aEvent, aVar)
  {
    ok(aVar._variablesView, "variables view object");

    findVariableViewProperties(aVar, [
      { name: "testProp", value: "testValue" },
      { name: "document", value: "HTMLDocument" },
    ], { webconsole: hud }).then(finishTest);
  }
}
