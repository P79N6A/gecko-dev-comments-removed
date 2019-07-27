






"use strict";

const TEST_URI = "data:text/html;charset=utf-8,<p>bug 900448 - autocomplete " +
                 "popup closes on tab switch";

let test = asyncTest(function*() {
  yield loadTab(TEST_URI);
  let hud = yield openConsole();
  let popup = hud.jsterm.autocompletePopup;
  let popupShown = onPopupShown(popup._panel);

  hud.jsterm.setInputValue("sc");
  EventUtils.synthesizeKey("r", {});

  yield popupShown;

  ok(!popup.isOpen, "Popup closes on tab switch");
});

function onPopupShown(panel) {
  let finished = promise.defer();

  panel.addEventListener("popupshown", function popupOpened() {
    panel.removeEventListener("popupshown", popupOpened, false);
    loadTab("data:text/html;charset=utf-8,<p>testing autocomplete closes")
      .then(finished.resolve);
  }, false);

  return finished.promise;
}
