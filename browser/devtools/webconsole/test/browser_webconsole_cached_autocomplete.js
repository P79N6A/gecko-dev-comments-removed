







const TEST_URI = "data:text/html;charset=utf8,<p>test cached autocompletion results";

let testDriver;

function test() {
  requestLongerTimeout(2);
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
  let popup = jsterm.autocompletePopup;

  
  input.value = "doc";
  input.setSelectionRange(3, 3);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  is(input.value, "doc", "'docu' completion (input.value)");
  is(jsterm.completeNode.value, "   ument", "'docu' completion (completeNode)");

  
  input.value = "window.";
  input.setSelectionRange(7, 7);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  ok(popup.getItems().length > 0, "'window.' gave a list of suggestions")

  content.wrappedJSObject.docfoobar = true;

  
  input.value = "window.doc";
  input.setSelectionRange(10, 10);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  let newItems = popup.getItems();
  ok(newItems.every(function(item) {
       return item.label != "docfoobar";
     }), "autocomplete cached results do not contain docfoobar. list has not been updated");

  
  input.value = "window.do";
  input.setSelectionRange(9, 9);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  newItems = popup.getItems();
  ok(newItems.every(function(item) {
       return item.label != "docfoobar";
     }), "autocomplete cached results do not contain docfoobar. list has not been updated");

  delete content.wrappedJSObject.docfoobar;

  
  input.value = "window."
  input.setSelectionRange(7, 7);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;
  input.value = "window.getC";
  input.setSelectionRange(11, 11);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;
  newItems = popup.getItems();
  ok(!newItems.every(function(item) {
       return item.label != "getComputedStyle";
     }), "autocomplete results do contain getComputedStyle");

  
  input.value = "dump(d";
  input.setSelectionRange(6, 6);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  ok(popup.getItems().length > 0, "'dump(d' gives non-zero results");

  
  input.value = "dump(window.)";
  input.setSelectionRange(12, 12);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  content.wrappedJSObject.docfoobar = true;

  
  input.value = "dump(window.doc)";
  input.setSelectionRange(15, 15);
  jsterm.complete(jsterm.COMPLETE_HINT_ONLY, testNext);
  yield undefined;

  newItems = popup.getItems();
  ok(newItems.every(function(item) {
       return item.label != "docfoobar";
     }), "autocomplete cached results do not contain docfoobar. list has not been updated");

  delete content.wrappedJSObject.docfoobar;

  testDriver = null;
  executeSoon(finishTest);
  yield undefined;
}
