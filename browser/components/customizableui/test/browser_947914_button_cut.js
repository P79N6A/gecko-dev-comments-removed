



"use strict";

let initialLocation = gBrowser.currentURI.spec;
let globalClipboard;

add_task(function() {
  info("Check cut button existence and functionality");

  let testText = "cut text test";

  gURLBar.focus();
  yield PanelUI.show();
  info("Menu panel was opened");

  let cutButton = document.getElementById("cut-button");
  ok(cutButton, "Cut button exists in Panel Menu");
  ok(!cutButton.hasAttribute("disabled"), "Cut button is enabled");

  
  gURLBar.value = testText;
  gURLBar.focus();
  gURLBar.select();
  yield PanelUI.show();
  info("Menu panel was opened");

  ok(!cutButton.hasAttribute("disabled"), "Cut button is enabled when selecting");
  cutButton.click();
  is(gURLBar.value, "", "Selected text is removed from source when clicking on cut");

  
  let clipboard = Services.clipboard;
  let transferable = Cc["@mozilla.org/widget/transferable;1"].createInstance(Ci.nsITransferable);
  globalClipboard = clipboard.kGlobalClipboard;

  transferable.init(null);
  transferable.addDataFlavor("text/unicode");
  clipboard.getData(transferable, globalClipboard);
  let str = {}, strLength = {};
  transferable.getTransferData("text/unicode", str, strLength);
  let clipboardValue = "";

  if (str.value) {
    str.value.QueryInterface(Ci.nsISupportsString);
    clipboardValue = str.value.data;
  }
  is(clipboardValue, testText, "Data was copied to the clipboard.");
});

add_task(function asyncCleanup() {
  
  Services.clipboard.emptyClipboard(globalClipboard);
  info("Clipboard was cleared");

  
  gBrowser.addTab(initialLocation);
  gBrowser.removeTab(gBrowser.selectedTab);
  info("Tabs were restored");
});
