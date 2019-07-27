







"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for " +
                 "bug 659907: Expand console object with a dir method";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();
  hud.jsterm.clearOutput();

  hud.jsterm.execute("console.dir(document)");

  let varView = yield hud.jsterm.once("variablesview-fetched");

  yield findVariableViewProperties(varView, [
    {
      name: "__proto__.__proto__.querySelectorAll",
      value: "querySelectorAll()"
    },
    {
      name: "location",
      value: /Location \u2192 data:Web/
    },
    {
      name: "__proto__.write",
      value: "write()"
    },
  ], { webconsole: hud });
});
