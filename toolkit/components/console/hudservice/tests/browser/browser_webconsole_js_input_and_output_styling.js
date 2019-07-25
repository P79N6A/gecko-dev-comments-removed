










































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
  ok(jsInputNode.classList.contains("webconsole-msg-input"),
     "JS input node is of the CSS class 'webconsole-msg-input'");

  let jsOutputNodes = jsterm.outputNode.
                      querySelectorAll(".webconsole-msg-output");
  isnot(jsOutputNodes[0].textContent.indexOf("4"), -1,
        "JS output node contains '4'");
  ok(jsOutputNodes[0].classList.contains("webconsole-msg-output"),
     "JS output node is of the CSS class 'webconsole-msg-output'");

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  finishTest();
}

