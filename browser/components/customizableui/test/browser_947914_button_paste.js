



"use strict";

let initialLocation = gBrowser.currentURI.spec;
let globalClipboard;

add_task(function() {
  info("Check paste button existence and functionality");

  let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
  globalClipboard = Services.clipboard.kGlobalClipboard;

  yield PanelUI.show();
  info("Menu panel was opened");

  let pasteButton = document.getElementById("paste-button");
  ok(pasteButton, "Paste button exists in Panel Menu");

  
  let text = "Sample text for testing";
  clipboard.copyString(text);

  
  gURLBar.focus();
  yield PanelUI.show();
  info("Menu panel was opened");

  ok(!pasteButton.hasAttribute("disabled"), "Paste button is enabled");
  pasteButton.click();

  is(gURLBar.value, text, "Text pasted successfully");
});

add_task(function asyncCleanup() {
  
  Services.clipboard.emptyClipboard(globalClipboard);
  info("Clipboard was cleared");

  
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(gBrowser.selectedTab);
  info("Tabs were restored");
});

