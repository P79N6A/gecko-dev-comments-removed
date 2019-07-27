



"use strict";

let initialLocation = gBrowser.currentURI.spec;
let globalClipboard;

add_task(function() {
  info("Check copy button existence and functionality");

  let testText = "copy text test";

  gURLBar.focus();
  info("The URL bar was focused");
  yield PanelUI.show();
  info("Menu panel was opened");

  let copyButton = document.getElementById("copy-button");
  ok(copyButton, "Copy button exists in Panel Menu");
  ok(copyButton.getAttribute("disabled"), "Copy button is initially disabled");

  
  gURLBar.value = testText;
  gURLBar.focus();
  gURLBar.select();
  yield PanelUI.show();
  info("Menu panel was opened");

  ok(!copyButton.hasAttribute("disabled"), "Copy button is enabled when selecting");

  copyButton.click();
  is(gURLBar.value, testText, "Selected text is unaltered when clicking copy");

  
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
