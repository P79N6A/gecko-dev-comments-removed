



"use strict";

add_task(function() {
  info("Check copy button existence and functionality");

  var testText = "copy text test";
  let initialLocation = gBrowser.currentURI.spec;

  yield PanelUI.show();

  let copyButton = document.getElementById("copy-button");
  ok(copyButton, "Copy button exists in Panel Menu");
  is(copyButton.getAttribute("disabled"), "true", "Copy button is initially disabled");

  
  gURLBar.value = testText;
  gURLBar.focus();
  gURLBar.select();
  yield PanelUI.show();

  ok(!copyButton.hasAttribute("disabled"), "Copy button gets enabled");

  copyButton.click();
  is(gURLBar.value, testText, "Selected text is unaltered when clicking copy");

  
  let clipboard = Services.clipboard;
  let transferable = Cc["@mozilla.org/widget/transferable;1"].createInstance(Ci.nsITransferable);
  const globalClipboard = clipboard.kGlobalClipboard;

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

  
  Services.clipboard.emptyClipboard(globalClipboard);
  gURLBar.value = initialLocation;
});
