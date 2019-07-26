



"use strict";

add_task(function() {
  info("Check paste button existence and functionality");

  let initialLocation = gBrowser.currentURI.spec;
  let clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper);
  const globalClipboard = Services.clipboard.kGlobalClipboard;

  yield PanelUI.show();

  let pasteButton = document.getElementById("paste-button");
  ok(pasteButton, "Paste button exists in Panel Menu");

  
  var text = "Sample text for testing";
  clipboard.copyString(text);

  
  gURLBar.focus();
  yield PanelUI.show();

  ok(!pasteButton.hasAttribute("disabled"), "Paste button is enabled");
  pasteButton.click();

  is(gURLBar.value, text, "Text pasted successfully");

  
  Services.clipboard.emptyClipboard(globalClipboard);
  gURLBar.value = initialLocation;
});
