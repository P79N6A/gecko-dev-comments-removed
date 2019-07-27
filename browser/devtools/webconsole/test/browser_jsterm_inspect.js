






"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>hello bug 869981";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  let jsterm = hud.jsterm;

  jsterm.execute("testProp = 'testValue'");

  let fetched = jsterm.once("variablesview-fetched");
  jsterm.execute("inspect(window)");
  let variable = yield fetched;

  ok(variable._variablesView, "variables view object");

  yield findVariableViewProperties(variable, [
    { name: "testProp", value: "testValue" },
    { name: "document", value: /HTMLDocument \u2192 data:/ },
  ], { webconsole: hud });
});
