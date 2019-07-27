






"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>hello";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();
  hud.jsterm.clearOutput();

  is(hud.jsterm.inputNode.hasAttribute("focused"), true,
     "inputNode should be focused");

  hud.ui.filterBox.focus();

  is(hud.ui.filterBox.hasAttribute("focused"), true,
     "filterBox should be focused");

  is(hud.jsterm.inputNode.hasAttribute("focused"), false,
     "inputNode shouldn't be focused");

  yield openDebugger();
  hud = yield openConsole();

  is(hud.jsterm.inputNode.hasAttribute("focused"), true,
     "inputNode should be focused");
});
