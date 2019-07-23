



































function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  let tabbrowser = gBrowser;
  waitForExplicitFinish();
  
  
  let max_tabs_undo = gPrefService.getIntPref("browser.sessionstore.max_tabs_undo");
  gPrefService.setIntPref("browser.sessionstore.max_tabs_undo", max_tabs_undo + 1);
  let closedTabCount = ss.getClosedTabCount(window);
  
  
  let tab = tabbrowser.addTab("about:");
  tab.linkedBrowser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    
    let history = tab.linkedBrowser.webNavigation.sessionHistory;
    ok(history.count >= 1, "the new tab does have at least one history entry");
    
    ss.setTabState(tab, "{ entries: [] }");
    tab.linkedBrowser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      ok(history.count == 0, "the tab was restored without any history whatsoever");
      
      tabbrowser.removeTab(tab);
      ok(ss.getClosedTabCount(window) == closedTabCount,
         "The closed blank tab wasn't added to Recently Closed Tabs");
      
      
      gPrefService.clearUserPref("browser.sessionstore.max_tabs_undo");
      finish();
    }, true);
  }, true);
}
