function test() {
  
  
  function test(aLambda) {
    try {
      return aLambda() || true;
    }
    catch (ex) { }
    return false;
  }
  
  
  let tabbrowser = gBrowser;
  waitForExplicitFinish();
  
  
  let ssComponent = test(function() Cc["@mozilla.org/browser/sessionstore;1"]);
  ok(ssComponent, "reference the sessionstore component");
  
  
  let ss = test(function() ssComponent.getService(Ci.nsISessionStore));
  ok(ss, "reference the sessionstore service");
  
  
  
  
  let key = "Unique name: " + Date.now();
  let value = "Unique value: " + Math.random();
  
  
  ok(test(function() ss.setWindowValue(window, key, value)), "set a window value");
  
  
  is(ss.getWindowValue(window, key), value, "stored window value matches original");
  
  
  ok(test(function() ss.deleteWindowValue(window, key)), "delete the window value");
  
  
  is(ss.getWindowValue(window, key), "", "window value was deleted");
  
  
  
  
  key = "Unique name: " + Math.random();
  value = "Unique value: " + Date.now();
  let tab = tabbrowser.addTab();
  tab.linkedBrowser.stop();
  
  
  ok(test(function() ss.setTabValue(tab, key, value)), "store a tab value");
  
  
  is(ss.getTabValue(tab, key), value, "stored tab value match original");
  
  
  ok(test(function() ss.deleteTabValue(tab, key)), "delete the tab value");
  
  is(ss.getTabValue(tab, key), "", "tab value was deleted");
  
  
  tabbrowser.removeTab(tab);
  
  
  
  
  
  
  let count = ss.getClosedTabCount(window);
  let max_tabs_undo = gPrefService.getIntPref("browser.sessionstore.max_tabs_undo");
  ok(0 <= count && count <= max_tabs_undo,
     "getClosedTabCount returns zero or at most max_tabs_undo");
  
  
  let testURL = "about:";
  tab = tabbrowser.addTab(testURL);
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    
    gPrefService.setIntPref("browser.sessionstore.max_tabs_undo", max_tabs_undo + 1);
    
    
    tabbrowser.removeTab(tab);
    
    
    var newcount = ss.getClosedTabCount(window);
    ok(newcount > count, "after closing a tab, getClosedTabCount has been incremented");
    
    
    tab = test(function() ss.undoCloseTab(window, 0));
    ok(tab, "undoCloseTab doesn't throw")
    
    tab.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      is(this.currentURI.spec, testURL, "correct tab was reopened");
      
      
      if (gPrefService.prefHasUserValue("browser.sessionstore.max_tabs_undo"))
        gPrefService.clearUserPref("browser.sessionstore.max_tabs_undo");
      tabbrowser.removeTab(tab);
      finish();
    }, true);
  }, true);
}
