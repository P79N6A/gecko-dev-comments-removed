




const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/test/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, testInputChange);
  }, true);
}

function testInputChange(hud) {
  var jsterm = hud.jsterm;
  var input = jsterm.inputNode;

  is(input.getAttribute("focused"), "true", "input has focus");
  EventUtils.synthesizeKey("VK_TAB", {});
  is(input.getAttribute("focused"), "", "focus moved away");

  
  input.focus();
  EventUtils.synthesizeKey("A", {});
  EventUtils.synthesizeKey("VK_TAB", {});
  is(input.getAttribute("focused"), "true", "input is still focused");

  
  input.blur();
  input.focus();
  EventUtils.synthesizeKey("VK_RIGHT", {});
  EventUtils.synthesizeKey("VK_TAB", {});
  is(input.getAttribute("focused"), "", "input moved away");

  jsterm = input = null;
  finishTest();
}
