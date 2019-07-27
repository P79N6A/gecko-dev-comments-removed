







"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>test cached autocompletion " +
                 "results";

let jsterm;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  jsterm = hud.jsterm;
  let input = jsterm.inputNode;
  let popup = jsterm.autocompletePopup;

  
  input.value = "doc";
  input.setSelectionRange(3, 3);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  is(input.value, "doc", "'docu' completion (input.value)");
  is(jsterm.completeNode.value, "   ument", "'docu' completion (completeNode)");

  
  input.value = "window.";
  input.setSelectionRange(7, 7);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  ok(popup.getItems().length > 0, "'window.' gave a list of suggestions");

  yield jsterm.execute("window.docfoobar = true");

  
  input.value = "window.doc";
  input.setSelectionRange(10, 10);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  let newItems = popup.getItems();
  ok(newItems.every(function(item) {
    return item.label != "docfoobar";
  }), "autocomplete cached results do not contain docfoobar. list has not " +
      "been updated");

  
  input.value = "window.do";
  input.setSelectionRange(9, 9);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  newItems = popup.getItems();
  ok(newItems.every(function(item) {
    return item.label != "docfoobar";
  }), "autocomplete cached results do not contain docfoobar. list has not " +
      "been updated");

  yield jsterm.execute("delete window.docfoobar");

  
  input.value = "window.";
  input.setSelectionRange(7, 7);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  input.value = "window.getC";
  input.setSelectionRange(11, 11);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  newItems = popup.getItems();
  ok(!newItems.every(function(item) {
       return item.label != "getComputedStyle";
     }), "autocomplete results do contain getComputedStyle");

  
  input.value = "dump(d";
  input.setSelectionRange(6, 6);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  ok(popup.getItems().length > 0, "'dump(d' gives non-zero results");

  
  input.value = "dump(window.)";
  input.setSelectionRange(12, 12);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  yield jsterm.execute("window.docfoobar = true");

  
  input.value = "dump(window.doc)";
  input.setSelectionRange(15, 15);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  newItems = popup.getItems();
  ok(newItems.every(function(item) {
    return item.label != "docfoobar";
  }), "autocomplete cached results do not contain docfoobar. list has not " +
      "been updated");

  yield jsterm.execute("delete window.docfoobar");

  jsterm = null;
});

function complete(type) {
  let updated = jsterm.once("autocomplete-updated");
  jsterm.complete(type);
  return updated;
}
