









"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/" +
                 "webconsole/test/test-bug-597136-external-script-" +
                 "errors.html";

function test() {
  Task.spawn(function* () {
    const {tab} = yield loadTab(TEST_URI);
    const hud = yield openConsole(tab);

    let button = content.document.querySelector("button");

    expectUncaughtException();
    EventUtils.sendMouseEvent({ type: "click" }, button, content);

    yield waitForMessages({
      webconsole: hud,
      messages: [{
        text: "bogus is not defined",
        category: CATEGORY_JS,
        severity: SEVERITY_ERROR,
      }],
    });
  }).then(finishTest);
}
