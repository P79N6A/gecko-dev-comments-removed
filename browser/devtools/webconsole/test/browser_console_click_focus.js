






const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testInputFocus, false);
}

function testInputFocus() {
  browser.removeEventListener("DOMContentLoaded", testInputFocus, false);

  openConsole().then((hud) => {
    let inputNode = hud.jsterm.inputNode;
    ok(inputNode.getAttribute("focused"), "input node is focused");

    let lostFocus = () => {
      inputNode.removeEventListener("blur", lostFocus);
      info("input node lost focus");
    }

   	inputNode.addEventListener("blur", lostFocus);

   	browser.ownerDocument.getElementById("urlbar").click();

  	ok(!inputNode.getAttribute("focused"), "input node is not focused");

   	hud.outputNode.click();

   	ok(inputNode.getAttribute("focused"), "input node is focused");

      finishTest();
  });
}

