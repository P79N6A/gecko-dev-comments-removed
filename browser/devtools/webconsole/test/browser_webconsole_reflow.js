




"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Web Console test for " +
                 "reflow activity";

let test = asyncTest(function* () {
  let { browser } = yield loadTab(TEST_URI);

  let hud = yield openConsole();

  function onReflowListenersReady() {
    browser.contentDocument.body.style.display = "none";
    browser.contentDocument.body.clientTop;
  }

  Services.prefs.setBoolPref("devtools.webconsole.filter.csslog", true);
  hud.ui._updateReflowActivityListener(onReflowListenersReady);
  Services.prefs.clearUserPref("devtools.webconsole.filter.csslog");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: /reflow: /,
      category: CATEGORY_CSS,
      severity: SEVERITY_LOG,
    }],
  });
});
