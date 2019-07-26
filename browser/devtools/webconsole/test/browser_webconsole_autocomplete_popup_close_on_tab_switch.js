






const TEST_URI = "data:text/html;charset=utf-8,<p>bug 900448 - autocomplete popup closes on tab switch";

let popup = null;

registerCleanupFunction(function() {
  popup = null;
});

function test() {
  addTab(TEST_URI);
  browser.addEventListener("load", function onLoad() {
    browser.removeEventListener("load", onLoad, true);
    openConsole(null, consoleOpened);
  }, true);
}

function consoleOpened(HUD) {
  popup = HUD.jsterm.autocompletePopup;

  popup._panel.addEventListener("popupshown", function popupOpened() {
    popup._panel.removeEventListener("popupshown", popupOpened, false);
    addTab("data:text/html;charset=utf-8,<p>testing autocomplete closes");
    gBrowser.selectedBrowser.addEventListener("load", tab2Loaded, true);
  }, false);

  HUD.jsterm.setInputValue("sc");
  EventUtils.synthesizeKey("r", {});
}

function tab2Loaded() {
  gBrowser.selectedBrowser.removeEventListener("load", tab2Loaded, true);
  ok(!popup.isOpen, "Popup closes on tab switch");
  gBrowser.removeCurrentTab();
  finishTest();
}
