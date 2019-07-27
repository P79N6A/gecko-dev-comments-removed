







"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-console.html";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();

  let jsterm = hud.jsterm;

  jsterm.clearOutput();
  jsterm.execute("console.log('foo', 'bar');");

  let [functionCall, result, consoleMessage] = yield waitForMessages({
    webconsole: hud,
    messages: [{
      text: "console.log('foo', 'bar');",
      category: CATEGORY_INPUT,
    },
    {
      text: "undefined",
      category: CATEGORY_OUTPUT,
    },
    {
      text: "foo bar",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  });

  let fncallNode = [...functionCall.matched][0];
  let resultNode = [...result.matched][0];
  let consoleMessageNode = [...consoleMessage.matched][0];
  is(fncallNode.nextElementSibling, resultNode,
     "console.log() is followed by undefined");
  is(resultNode.nextElementSibling, consoleMessageNode,
     "undefined is followed by 'foo' 'bar'");
});
