






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console-assert.html";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();
  yield consoleOpened(hud);
});

function consoleOpened(hud) {
  hud.jsterm.execute("test()");

  return waitForMessages({
    webconsole: hud,
    messages: [{
      text: "undefined",
      category: CATEGORY_OUTPUT,
      severity: SEVERITY_LOG,
    },
    {
      text: "start",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    },
    {
      text: "false assert",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_ERROR,
    },
    {
      text: "falsy assert",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_ERROR,
    },
    {
      text: "end",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(() => {
    let nodes = hud.outputNode.querySelectorAll(".message");
    is(nodes.length, 6, "only six messages are displayed, no output from the true assert");
  });
}
