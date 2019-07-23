



































function test() {
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();
  
  let uniqueValue = Math.random() + "\u2028Second line\u2029Second paragraph\u2027";
  
  let tab = gBrowser.addTab();
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    ss.setTabValue(tab, "bug485563", uniqueValue);
    let tabState = eval("(" + ss.getTabState(tab) + ")");
    is(tabState.extData["bug485563"], uniqueValue,
       "unicode line separator wasn't over-encoded");
    ss.deleteTabValue(tab, "bug485563");
    ss.setTabState(tab, tabState.toSource());
    is(ss.getTabValue(tab, "bug485563"), uniqueValue,
       "unicode line separator was correctly preserved");
    
    gBrowser.removeTab(tab);
    finish();
  }, true);
}
