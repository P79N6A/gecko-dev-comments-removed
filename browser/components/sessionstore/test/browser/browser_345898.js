



































function test() {
  
  
  function test(aLambda) {
    try {
      aLambda();
      return false;
    }
    catch (ex) {
      return ex.name == "NS_ERROR_ILLEGAL_VALUE";
    }
  }
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  
  
  ok(test(function() ss.getWindowState({})),
     "Invalid window for getWindowState throws");
  ok(test(function() ss.setWindowState({}, "", false)),
     "Invalid window for setWindowState throws");
  ok(test(function() ss.getTabState({})),
     "Invalid tab for getTabState throws");
  ok(test(function() ss.setTabState({}, "{}")),
     "Invalid tab state for setTabState throws");
  ok(test(function() ss.setTabState({}, "{ entries: [] }")),
     "Invalid tab for setTabState throws");
  ok(test(function() ss.duplicateTab({}, {})),
     "Invalid tab for duplicateTab throws");
  ok(test(function() ss.duplicateTab({}, gBrowser.selectedTab)),
     "Invalid window for duplicateTab throws");
  ok(test(function() ss.getClosedTabData({})),
     "Invalid window for getClosedTabData throws");
  ok(test(function() ss.undoCloseTab({}, 0)),
     "Invalid window for undoCloseTab throws");
  ok(test(function() ss.undoCloseTab(window, -1)),
     "Invalid index for undoCloseTab throws");
  ok(test(function() ss.getWindowValue({}, "")),
     "Invalid window for getWindowValue throws");
  ok(test(function() ss.getWindowValue({}, "")),
     "Invalid window for getWindowValue throws");
  ok(test(function() ss.getWindowValue({}, "", "")),
     "Invalid window for setWindowValue throws");
  ok(test(function() ss.deleteWindowValue({}, "")),
     "Invalid window for deleteWindowValue throws");
  ok(test(function() ss.deleteWindowValue(window, Date.now().toString())),
     "Nonexistent value for deleteWindowValue throws");
  ok(test(function() ss.deleteTabValue(gBrowser.selectedTab, Date.now().toString())),
     "Nonexistent value for deleteTabValue throws");
}
