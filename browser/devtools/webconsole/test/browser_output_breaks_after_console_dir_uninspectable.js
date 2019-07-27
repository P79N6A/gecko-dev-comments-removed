






"use strict";

const TEST_URI = "data:text/html;charset=utf8,test for bug 773466";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  hud.jsterm.clearOutput(true);

  hud.jsterm.execute("console.log('fooBug773466a')");
  hud.jsterm.execute("myObj = Object.create(null)");
  hud.jsterm.execute("console.dir(myObj)");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "fooBug773466a",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    },
    {
      name: "console.dir output",
      consoleDir: "[object Object]",
    }],
  });

  content.console.log("fooBug773466b");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "fooBug773466b",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });
});
