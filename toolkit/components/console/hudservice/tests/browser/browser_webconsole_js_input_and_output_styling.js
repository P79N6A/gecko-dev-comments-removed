










































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testJSInputAndOutputStyling,
                           false);
}

function testJSInputAndOutputStyling() {
  browser.removeEventListener("DOMContentLoaded",
                              testJSInputAndOutputStyling, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  let jsterm = HUDService.hudReferences[hudId].jsterm;

  jsterm.clearOutput();
  jsterm.execute("2 + 2");

  let nodes = jsterm.outputNode.querySelectorAll(".hud-msg-node");
  let jsInputNode = nodes[0];
  isnot(jsInputNode.textContent.indexOf("2 + 2"), -1,
    "JS input node contains '2 + 2'");
  isnot(jsInputNode.getAttribute("class").indexOf("jsterm-input-line"), -1,
    "JS input node is of the CSS class 'jsterm-input-line'");

  let jsOutputNodes = jsterm.outputNode.querySelectorAll(".jsterm-output-line");
  isnot(jsOutputNodes[0].textContent.indexOf("4"), -1, "JS output node contains '4'");
  isnot(jsOutputNodes[0].getAttribute("class").indexOf("jsterm-output-line"), -1,
    "JS output node is of the CSS class 'jsterm-output-line'");

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  finishTest();
}

