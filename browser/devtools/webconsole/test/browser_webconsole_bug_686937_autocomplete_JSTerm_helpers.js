






"use strict";

const TEST_URI = "data:text/html;charset=utf8,<p>test JSTerm Helpers " +
                 "autocomplete";

let jsterm;

let test = asyncTest(function* () {
  yield loadTab(TEST_URI);

  let hud = yield openConsole();

  jsterm = hud.jsterm;
  let input = jsterm.inputNode;
  let popup = jsterm.autocompletePopup;

  
  input.value = "i";
  input.setSelectionRange(1, 1);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  let newItems = popup.getItems().map(function(e) {
    return e.label;
  });
  ok(newItems.indexOf("inspect") > -1,
     "autocomplete results contain helper 'inspect'");

  
  input.value = "window.";
  input.setSelectionRange(7, 7);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  newItems = popup.getItems().map(function(e) {
    return e.label;
  });
  is(newItems.indexOf("inspect"), -1,
     "autocomplete results do not contain helper 'inspect'");

  
  input.value = "dump(i";
  input.setSelectionRange(6, 6);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  newItems = popup.getItems().map(function(e) {
    return e.label;
  });
  ok(newItems.indexOf("inspect") > -1,
     "autocomplete results contain helper 'inspect'");

  
  input.value = "window.dump(i";
  input.setSelectionRange(13, 13);
  yield complete(jsterm.COMPLETE_HINT_ONLY);

  newItems = popup.getItems().map(function(e) {
    return e.label;
  });
  ok(newItems.indexOf("inspect") > -1,
     "autocomplete results contain helper 'inspect'");

  jsterm = null;
});

function complete(type) {
  let updated = jsterm.once("autocomplete-updated");
  jsterm.complete(type);
  return updated;
}
