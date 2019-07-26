






const TEST_URI = "data:text/html;charset=utf8,<p>test code completion";

let testDriver;

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, function(hud) {
      testDriver = testCompletion(hud);
      testDriver.next();
    });
  }, true);
}

function testNext() {
  executeSoon(function() {
    testDriver.next();
  });
}

function testCompletion(hud) {
  let jsterm = hud.jsterm;
  let input = jsterm.inputNode;

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield;

  is(input.value, "docu", "'docu' completion (input.value)");
  is(jsterm.completeNode.value, "    ment", "'docu' completion (completeNode)");

  
  input.value = "docu";
  input.setSelectionRange(4, 4);
  jsterm.complete(jsterm.COMPLETE_FORWARD, testNext);
  yield;

  is(input.value, "document", "'docu' tab completion");
  is(input.selectionStart, 8, "start selection is alright");
  is(input.selectionEnd, 8, "end selection is alright");
  is(jsterm.completeNode.value.replace(/ /g, ""), "", "'docu' completed");

  
  
  input.value = "window.Ob";
  input.setSelectionRange(9, 9);
  jsterm.complete(jsterm.COMPLETE_FORWARD, testNext);
  yield;

  is(input.value, "window.Object", "'window.Ob' tab completion");

  
  input.value = "document.getElem";
  input.setSelectionRange(16, 16);
  jsterm.complete(jsterm.COMPLETE_FORWARD, testNext);
  yield;

  is(input.value, "document.getElem", "'document.getElem' completion");
  is(jsterm.completeNode.value, "", "'document.getElem' completion");

  
  jsterm.complete(jsterm.COMPLETE_FORWARD, testNext);
  yield;

  is(input.value, "document.getElem", "'document.getElem' completion");
  is(jsterm.completeNode.value, "                entsByTagNameNS", "'document.getElem' another tab completion");

  
  jsterm.complete(jsterm.COMPLETE_BACKWARD, testNext);
  yield;

  is(input.value, "document.getElem", "'document.getElem' untab completion");
  is(jsterm.completeNode.value, "", "'document.getElem' completion");

  jsterm.clearOutput();

  input.value = "docu";
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield;

  is(jsterm.completeNode.value, "    ment", "'docu' completion");
  jsterm.execute();
  is(jsterm.completeNode.value, "", "clear completion on execute()");

  
  input.value =                 "console.log('one');\nconsol";
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield;

  is(jsterm.completeNode.value, "                   \n      e", "multi-line completion");

  
  input.value = "Object.name.sl";
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield;

  is(jsterm.completeNode.value, "              ice", "non-object completion");

  
  input.value = "'Asimov'.sl";
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield;

  is(jsterm.completeNode.value, "           ice", "string literal completion");

  testDriver = jsterm = input = null;
  executeSoon(finishTest);
  yield;
}

