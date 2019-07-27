






"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>test code completion";

let jsterm;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  jsterm = hud.jsterm;
  let input = jsterm.inputNode;

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  is(input.value, "docu", "'docu' completion (input.value)");
  is(jsterm.completeNode.value, "    ment", "'docu' completion (completeNode)");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  yield complete(jsterm.COMPLETE_FORWARD);

  is(input.value, "document", "'docu' tab completion");
  is(input.selectionStart, 8, "start selection is alright");
  is(input.selectionEnd, 8, "end selection is alright");
  is(jsterm.completeNode.value.replace(/ /g, ""), "", "'docu' completed");

  
  
  input.value = "window.Ob";
  input.setSelectionRange(9, 9);
  yield complete(jsterm.COMPLETE_FORWARD);

  is(input.value, "window.Object", "'window.Ob' tab completion");

  
  input.value = "document.getElem";
  input.setSelectionRange(16, 16);
  yield complete(jsterm.COMPLETE_FORWARD);

  is(input.value, "document.getElem", "'document.getElem' completion");
  is(jsterm.completeNode.value, "                entsByTagNameNS",
     "'document.getElem' completion");

  
  yield jsterm.complete(jsterm.COMPLETE_FORWARD);

  is(input.value, "document.getElem", "'document.getElem' completion");
  is(jsterm.completeNode.value, "                entsByTagName",
     "'document.getElem' another tab completion");

  
  complete(jsterm.COMPLETE_BACKWARD);

  is(input.value, "document.getElem", "'document.getElem' untab completion");
  is(jsterm.completeNode.value, "                entsByTagNameNS",
     "'document.getElem' completion");

  jsterm.clearOutput();

  input.value = "docu";
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  is(jsterm.completeNode.value, "    ment", "'docu' completion");
  yield jsterm.execute();
  is(jsterm.completeNode.value, "", "clear completion on execute()");

  
  input.value = "console.log('one');\nconsol";
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  is(jsterm.completeNode.value, "                   \n      e",
     "multi-line completion");

  
  input.value = "Object.name.sl";
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  is(jsterm.completeNode.value, "              ice", "non-object completion");

  
  input.value = "'Asimov'.sl";
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  is(jsterm.completeNode.value, "           ice", "string literal completion");

  jsterm = null;
});

function complete(type) {
  let updated = jsterm.once("autocomplete-updated");
  jsterm.complete(type);
  return updated;
}
