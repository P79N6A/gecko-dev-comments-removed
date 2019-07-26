



function test() {
  
  let originalTab = gBrowser.selectedTab;
  let newTab1 = gBrowser.addTab();
  let newTab2 = gBrowser.addTab();
  gBrowser.pinTab(newTab1);

  
  is(gBrowser.getTabsToTheEndFrom(originalTab).length, 1,
    "One unpinned tab to the right");

  
  gBrowser.removeTabsToTheEndFrom(originalTab);
  is(gBrowser.tabs.length, 2, "Length is 2");
  is(gBrowser.tabs[1], originalTab, "Starting tab is not removed");
  is(gBrowser.tabs[0], newTab1, "Pinned tab is not removed");

  
  gBrowser.removeTab(newTab1);
}
