









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testInputFocus, false);
}

function testInputFocus() {
  browser.removeEventListener("DOMContentLoaded", testInputFocus, false);

  openConsole();

  let inputNode = HUDService.getHudByWindow(content).jsterm.inputNode;
  ok(inputNode.getAttribute("focused"), "input node is focused");

  finishTest();
}

