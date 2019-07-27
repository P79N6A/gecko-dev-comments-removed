var theTab;
var theBrowser;

function listener(evt) {
  if (evt.target == theBrowser.contentDocument) {
    doTest();
  }
}

function test() {
  waitForExplicitFinish();
  var testURL = getRootDirectory(gTestPath) + "newtab_share_rule_processors.html";
  theTab = gBrowser.addTab(testURL);
  theBrowser = gBrowser.getBrowserForTab(theTab);
  theBrowser.addEventListener("load", listener, true);
}

function doTest() {
  theBrowser.removeEventListener("load", listener, true);
  var winUtils = theBrowser.contentWindow
    .QueryInterface(Ci.nsIInterfaceRequestor)
    .getInterface(Ci.nsIDOMWindowUtils);
  
  
  ok(winUtils.hasRuleProcessorUsedByMultipleStyleSets(Ci.nsIStyleSheetService.AGENT_SHEET),
     "agent sheet rule processor is used by multiple style sets");
  
  ok(!winUtils.hasRuleProcessorUsedByMultipleStyleSets(Ci.nsIStyleSheetService.AUTHOR_SHEET),
     "author sheet rule processor is not used by multiple style sets");
  
  
  theBrowser.contentWindow.wrappedJSObject.addAgentSheet();
  ok(!winUtils.hasRuleProcessorUsedByMultipleStyleSets(Ci.nsIStyleSheetService.AGENT_SHEET),
     "agent sheet rule processor is not used by multiple style sets after " +
     "having a unique sheet added to it");
  gBrowser.removeTab(theTab);
  finish();
}
