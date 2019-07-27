







"use strict";

thisTestLeaksUncaughtRejectionsAndShouldBeFixed("null");



const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console-output-events.html";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  hud.jsterm.clearOutput();
  hud.jsterm.execute("testDOMEvents()");

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      name: "testDOMEvents() output",
      text: "undefined",
      category: CATEGORY_OUTPUT,
    }],
  });

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      name: "console.log() output for mousemove",
      text: /eventLogger mousemove { target: .+, buttons: 0, clientX: \d+, clientY: \d+, layerX: \d+, layerY: \d+ }/,
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  yield waitForMessages({
    webconsole: hud,
    messages: [{
      name: "console.log() output for keypress",
      text: /eventLogger keypress Shift { target: .+, key: .+, charCode: \d+, keyCode: \d+ }/,
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });
});
