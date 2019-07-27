








"use strict";

const TEST_URI = "data:text/html;charset=utf8,Web Console CSP violation test";
const TEST_VIOLATION = "https://example.com/browser/browser/devtools/" +
                       "webconsole/test/test_bug_770099_violation.html";
const CSP_VIOLATION_MSG = "Content Security Policy: The page's settings " +
                          "blocked the loading of a resource at " +
                          "http://some.example.com/test.png (\"default-src " +
                            "https://example.com\").";

let test = asyncTest(function* () {
  let { browser } = yield loadTab(TEST_URI);

  let hud = yield openConsole();

  hud.jsterm.clearOutput();

  let loaded = loadBrowser(browser);
  content.location = TEST_VIOLATION;
  yield loaded;

  yield waitForSuccess({
    name: "CSP policy URI warning displayed successfully",
    validator: function() {
      return hud.outputNode.textContent.indexOf(CSP_VIOLATION_MSG) > -1;
    }
  });
});
