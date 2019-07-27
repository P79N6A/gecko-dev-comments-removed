






"use strict";

function test() {
  Task.spawn(runner).then(finishTest);

  function* runner() {
    const {tab} = yield loadTab("data:text/html;charset=utf8,<p>hello");

    const hud = yield openConsole(tab);

    hud.jsterm.execute("console.log('foobar bug970914')");

    yield waitForMessages({
      webconsole: hud,
      messages: [{
        name: "console.log",
        text: "foobar bug970914",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      }],
    });

    let text = hud.outputNode.textContent;
    isnot(text.indexOf("foobar bug970914"), -1,
          "console.log message confirmed");
    ok(!/logging API|disabled by a script/i.test(text),
       "no warning about disabled console API");
  }
}

