








"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-error.html";

function test() {
  Task.spawn(function*() {
    const {tab} = yield loadTab(TEST_URI);
    const hud = yield openConsole(tab);
    info("console opened");

    executeSoon(() => {
      hud.jsterm.clearOutput();
      info("wait for reload");
      content.location.reload();
    });

    yield hud.target.once("navigate");
    info("target navigated");

    let button = content.document.querySelector("button");
    ok(button, "button found");

    expectUncaughtException();
    EventUtils.sendMouseEvent({type: "click"}, button, content);

    yield waitForMessages({
      webconsole: hud,
      messages: [{
        text: "fooBazBaz is not defined",
        category: CATEGORY_JS,
        severity: SEVERITY_ERROR,
      }],
    });
  }).then(finishTest);
}
