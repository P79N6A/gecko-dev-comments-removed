



































function test() {
  let newTab = gBrowser.addTab();
  waitForExplicitFinish();
  newTab.linkedBrowser.addEventListener("load", mainPart, true);
  
  function mainPart() {
    gBrowser.pinTab(newTab);
    gBrowser.selectedTab = newTab;
    
    openUILinkIn("javascript:var x=0;", "current");
    is(gBrowser.tabs.length, 2, "Should open in current tab");
    
    openUILinkIn("http://www.example.com/1", "current");
    is(gBrowser.tabs.length, 2, "Should open in current tab");
    
    openUILinkIn("http://www.example.org/", "current");
    is(gBrowser.tabs.length, 3, "Should open in new tab");
    
    newTab.removeEventListener("load", mainPart, true);
    gBrowser.removeTab(newTab);
    gBrowser.removeTab(gBrowser.tabs[1]); 
    finish();
  }
  newTab.linkedBrowser.loadURI("http://www.example.com");
}
