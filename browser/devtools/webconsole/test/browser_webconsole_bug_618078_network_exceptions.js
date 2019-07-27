







"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for bug 618078";
const TEST_URI2 = "http://example.com/browser/browser/devtools/webconsole/" +
                  "test/test-bug-618078-network-exceptions.html";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  expectUncaughtException();

  content.location = TEST_URI2;

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "bug618078exception",
      category: CATEGORY_JS,
      severity: SEVERITY_ERROR,
    }],
  });
});
