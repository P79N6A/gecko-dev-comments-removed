




"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/browser/test-console.html";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  let jsterm = hud.jsterm;
  let input = jsterm.inputNode;

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
});
