





































const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test//test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testInputExpansion, false);
}

function testInputExpansion() {
  browser.removeEventListener("DOMContentLoaded", testInputExpansion, false);

  openConsole();

  let input = HUDService.getHudByWindow(content).jsterm.inputNode;

  input.focus();

  is(input.getAttribute("multiline"), "true", "multiline is enabled");

  let ordinaryHeight = input.clientHeight;

  
  input.value = "hello\nworld\n";
  let length = input.value.length;
  input.selectionEnd = length;
  input.selectionStart = length;
  
  
  EventUtils.synthesizeKey("d", {});
  ok(input.clientHeight > ordinaryHeight, "the input expanded");

  
  input.value = "";
  EventUtils.synthesizeKey("d", {});
  is(input.clientHeight, ordinaryHeight, "the input's height is normal again");

  input = length = null;

  finishTest();
}

