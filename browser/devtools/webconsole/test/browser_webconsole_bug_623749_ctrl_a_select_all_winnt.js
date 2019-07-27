





"use strict";

const TEST_URI = "data:text/html;charset=utf-8,Test console for bug 623749";

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  let jsterm = hud.jsterm;
  jsterm.setInputValue("Ignore These Four Words");
  let inputNode = jsterm.inputNode;

  
  EventUtils.synthesizeKey("a", { ctrlKey: true });
  let inputLength = inputNode.selectionEnd - inputNode.selectionStart;
  is(inputLength, inputNode.value.length, "Select all of input");

  
  jsterm.setInputValue("Ignore These Four Words");
  inputNode.selectionStart = 0;
  EventUtils.synthesizeKey("e", { ctrlKey: true });
  is(inputNode.selectionStart, 0, "Control + E does not move to end of input");
});
