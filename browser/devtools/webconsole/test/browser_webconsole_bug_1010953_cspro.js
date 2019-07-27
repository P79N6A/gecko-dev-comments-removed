
















"use strict";

const TEST_URI = "data:text/html;charset=utf8,Web Console CSP report only " +
                 "test (bug 1010953)";
const TEST_VIOLATION = "http://example.com/browser/browser/devtools/" +
                       "webconsole/test/test_bug_1010953_cspro.html";
const CSP_VIOLATION_MSG = "Content Security Policy: The page's settings " +
                          "blocked the loading of a resource at " +
                          "http://some.example.com/test.png " +
                          "(\"img-src http://example.com\").";
const CSP_REPORT_MSG = "Content Security Policy: The page\'s settings " +
                       "observed the loading of a resource at " +
                       "http://some.example.com/test_bug_1010953_cspro.js " +
                       "(\"script-src http://example.com\"). A CSP report is " +
                       "being sent.";

let test = asyncTest(function* () {
  let { browser } = yield loadTab(TEST_URI);

  let hud = yield openConsole();

  hud.jsterm.clearOutput();

  let loaded = loadBrowser(browser);
  content.location = TEST_VIOLATION;
  yield loaded;

  yield waitForSuccess({
    name: "Confirmed that CSP and CSP-Report-Only log different messages to " +
          "the console.",
    validator: function() {
      console.log(hud.outputNode.textContent);
      let success = false;
      success = hud.outputNode.textContent.indexOf(CSP_VIOLATION_MSG) > -1 &&
                hud.outputNode.textContent.indexOf(CSP_REPORT_MSG) > -1;
      return success;
    }
  });
});
