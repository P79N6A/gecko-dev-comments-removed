







































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testCompletion, false);
}

function testCompletion() {
  browser.removeEventListener("DOMContentLoaded", testCompletion, false);

  openConsole();

  var jsterm = HUDService.getHudByWindow(content).jsterm;
  var input = jsterm.inputNode;

  
  jsterm.setInputValue("var d = ");
  EventUtils.synthesizeKey("5", {});
  EventUtils.synthesizeKey(";", {});
  is(input.value, "var d = 5;", "var d = 5;");
  is(jsterm.completeNode.value, "", "no completion");
  EventUtils.synthesizeKey("VK_ENTER", {});
  is(jsterm.completeNode.value, "", "clear completion on execute()");
  
  
  jsterm.setInputValue("var a = ");
  EventUtils.synthesizeKey("d", {});
  is(input.value, "var a = d", "var a = d");
  is(jsterm.completeNode.value, "", "no completion");
  EventUtils.synthesizeKey("VK_ENTER", {});
  is(jsterm.completeNode.value, "", "clear completion on execute()");
  
  jsterm = input = null;
  finishTest();
}

