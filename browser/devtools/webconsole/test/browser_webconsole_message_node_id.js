




"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();

  hud.jsterm.execute("console.log('a log message')");

  let [result] = yield waitForMessages({
      webconsole: hud,
      messages: [{
        text: "a log message",
        category: CATEGORY_WEBDEV,
        severity: SEVERITY_LOG,
      }],
  });

  let msg = [...result.matched][0];
  ok(msg.getAttribute("id"), "log message has an ID");
});
