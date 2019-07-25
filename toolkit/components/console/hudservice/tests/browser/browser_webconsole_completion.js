









































const TEST_URI = "http://example.com/browser/toolkit/components/console/hudservice/tests/browser/test-console.html";

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", testCompletion, false);
}

function testCompletion() {
  browser.removeEventListener("DOMContentLoaded", testCompletion, false);

  openConsole();

  hudId = HUDService.displaysIndex()[0];

  var HUD = HUDService.hudWeakReferences[hudId].get();
  var jsterm = HUD.jsterm;
  var input = jsterm.inputNode;

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  is(input.value, "document", "'docu' completion");
  is(input.selectionStart, 4, "start selection is alright");
  is(input.selectionEnd, 8, "end selection is alright");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_FORWARD);
  is(input.value, "document", "'docu' tab completion");
  is(input.selectionStart, 8, "start selection is alright");
  is(input.selectionEnd, 8, "end selection is alright");

  
  input.value = "document.getElem";
  input.setSelectionRange(16, 16);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY);
  is(input.value, "document.getElementById", "'document.getElem' completion");
  is(input.selectionStart, 16, "start selection is alright");
  is(input.selectionEnd, 23, "end selection is alright");

  
  jsterm.complete(jsterm.COMPLETE_FORWARD);
  is(input.value, "document.getElementsByClassName", "'document.getElem' another tab completion");
  is(input.selectionStart, 16, "start selection is alright");
  is(input.selectionEnd, 31, "end selection is alright");

  
  jsterm.complete(jsterm.COMPLETE_BACKWARD);
  is(input.value, "document.getElementById", "'document.getElem' untab completion");
  is(input.selectionStart, 16, "start selection is alright");
  is(input.selectionEnd, 23, "end selection is alright");

  jsterm.clearOutput();
  jsterm.history.splice(0);   

  HUD = jsterm = input = null;
  finishTest();
}

