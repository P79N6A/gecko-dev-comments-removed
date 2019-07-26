






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testExecutionScope);
  }, true);
}

function testExecutionScope(hud) {
  let jsterm = hud.jsterm;

  jsterm.clearOutput();
  jsterm.execute("window.location.href;");
  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "window.location.href;",
      category: CATEGORY_INPUT,
    },
    {
      text: TEST_URI,
      category: CATEGORY_OUTPUT,
    }],
  }).then(([input, output]) => {
    let inputNode = [...input.matched][0];
    let outputNode = [...output.matched][0];
    is(inputNode.getAttribute("category"), "input", "input node category is correct");
    is(outputNode.getAttribute("category"), "output", "output node category is correct");
    finishTest();
  });
}

