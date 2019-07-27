







"use strict";

const TEST_NETWORK_URI = "http://example.com/browser/browser/devtools/" +
                         "webconsole/test/test-network.html" + "?_date=" +
                         Date.now();

let test = asyncTest(function* () {
  yield loadTab("data:text/html;charset=utf-8,Web Console basic network " +
                "logging test");
  let hud = yield openConsole();

  content.location = TEST_NETWORK_URI;

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "running network console",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    },
    {
      text: "test-network.html",
      category: CATEGORY_NETWORK,
      severity: SEVERITY_LOG,
    },
    {
      text: "testscript.js",
      category: CATEGORY_NETWORK,
      severity: SEVERITY_LOG,
    },
    {
      text: "test-image.png",
      category: CATEGORY_NETWORK,
      severity: SEVERITY_LOG,
    }],
  });
});
