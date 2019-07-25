









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testCompletion, false);
}

function testCompletion() {
  browser.removeEventListener("DOMContentLoaded", testCompletion, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  var HUD = HUDService.hudReferences[hudId];
  var jsterm = HUD.jsterm;
  var input = jsterm.inputNode;

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  is(input.value, "docu", "'docu' completion");
  is(jsterm.completeNode.value, "    ment", "'docu' completion");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_FORWARD);
  is(input.value, "document", "'docu' tab completion");
  is(input.selectionStart, 8, "start selection is alright");
  is(input.selectionEnd, 8, "end selection is alright");
  is(jsterm.completeNode.value.replace(/ /g, ""), "", "'docu' completed");

  
  input.value = "document.getElem";
  input.setSelectionRange(16, 16);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  is(input.value, "document.getElem", "'document.getElem' completion");
  is(jsterm.completeNode.value, "                entById", "'document.getElem' completion");

  
  jsterm.complete(jsterm.COMPLETE_FORWARD);
  is(input.value, "document.getElem", "'document.getElem' completion");
  is(jsterm.completeNode.value, "                entsByClassName", "'document.getElem' another tab completion");

  
  jsterm.complete(jsterm.COMPLETE_BACKWARD);
  is(input.value, "document.getElem", "'document.getElem' untab completion");
  is(jsterm.completeNode.value, "                entById", "'document.getElem' completion");

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  HUD = jsterm = input = null;
  finishTest();
}

