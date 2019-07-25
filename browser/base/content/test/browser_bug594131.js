



































function test() {
  let backgroundPref = "browser.tabs.loadBookmarksInBackground";
  let newTab = gBrowser.addTab("http://example.com");
  waitForExplicitFinish();
  newTab.linkedBrowser.addEventListener("load", mainPart, true);
  
  Services.prefs.setBoolPref(backgroundPref, true);
  
  function mainPart() {
    gBrowser.pinTab(newTab);
    gBrowser.selectedTab = newTab;
    
    openUILinkIn("http://example.org/", "current");
    isnot(gBrowser.selectedTab, newTab, "shouldn't load in background");
    
    if (Services.prefs.prefHasUserValue(backgroundPref))
      Services.prefs.clearUserPref(backgroundPref);
    gBrowser.removeTab(newTab);
    gBrowser.removeTab(gBrowser.tabs[1]); 
    finish();
  }
}
