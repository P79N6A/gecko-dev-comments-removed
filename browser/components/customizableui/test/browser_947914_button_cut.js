



"use strict";

add_task(function() {
  info("Check cut button existence and functionality");

  var testText = "cut text test";
  let initialLocation = gBrowser.currentURI.spec;

  yield PanelUI.show();

  let cutButton = document.getElementById("cut-button");
  ok(cutButton, "Cut button exists in Panel Menu");
  ok(cutButton.getAttribute("disabled"), "Cut button is disabled");

  
  gURLBar.value = testText;
  gURLBar.focus();
  gURLBar.select();
  yield PanelUI.show();

  ok(!cutButton.hasAttribute("disabled"), "Cut button gets enabled");
  cutButton.click();
  is(gURLBar.value, "", "Selected text is removed from source when clicking on cut");

  
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

  
  gBrowser.value = initialLocation;
  Services.clipboard.emptyClipboard(globalClipboard);
});
