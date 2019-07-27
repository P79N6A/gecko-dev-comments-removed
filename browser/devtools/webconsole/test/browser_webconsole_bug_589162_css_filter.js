










"use strict";

const TEST_URI = "data:text/html;charset=utf-8,<div style='font-size:3em;" +
                 "foobarCssParser:baz'>test CSS parser filter</div>";





function test() {
  Task.spawn(runner).then(finishTest);

  function* runner() {
    let {tab} = yield loadTab(TEST_URI);
    let hud = yield openConsole(tab);

    
    hud.setFilterState("cssparser", true);
    hud.jsterm.clearOutput();

    content.location.reload();

    yield waitForMessages({
      webconsole: hud,
      messages: [{
        text: "foobarCssParser",
        category: CATEGORY_CSS,
        severity: SEVERITY_WARNING,
      }],
    });

    hud.setFilterState("cssparser", false);

    let msg = "the unknown CSS property warning is not displayed, " +
              "after filtering";
    testLogEntry(hud.outputNode, "foobarCssParser", msg, true, true);
  }
}
