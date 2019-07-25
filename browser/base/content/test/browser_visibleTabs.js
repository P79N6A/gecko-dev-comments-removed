




































function test() {
  
  let [origTab] = gBrowser.visibleTabs;

  
  let pinned = gBrowser.addTab();
  gBrowser.pinTab(pinned);

  let testTab = gBrowser.addTab();

  let visible = gBrowser.visibleTabs;
  is(visible.length, 3, "3 tabs should be open");
  is(visible[0], pinned, "the pinned tab is first");
  is(visible[1], origTab, "original tab is next");
  is(visible[2], testTab, "last created tab is last");

  
  is(gBrowser.selectedTab, origTab, "sanity check that we're on the original tab");
  gBrowser.showOnlyTheseTabs([testTab]);
  is(gBrowser.visibleTabs.length, 3, "all 3 tabs are still visible");

  
  gBrowser.selectedTab = testTab;
  gBrowser.showOnlyTheseTabs([testTab]);

  
  
  
  let tabViewWindow = TabView.getContentWindow();
  if (tabViewWindow)
    tabViewWindow.GroupItems.moveTabToGroupItem(origTab, null);

  visible = gBrowser.visibleTabs;
  is(visible.length, 2, "2 tabs should be visible including the pinned");
  is(visible[0], pinned, "first is pinned");
  is(visible[1], testTab, "next is the test tab");
  is(gBrowser.tabs.length, 3, "3 tabs should still be open");

  gBrowser.selectTabAtIndex(0);
  is(gBrowser.selectedTab, pinned, "first tab is pinned");
  gBrowser.selectTabAtIndex(1);
  is(gBrowser.selectedTab, testTab, "second tab is the test tab");
  gBrowser.selectTabAtIndex(2);
  is(gBrowser.selectedTab, testTab, "no third tab, so no change");
  gBrowser.selectTabAtIndex(0);
  is(gBrowser.selectedTab, pinned, "switch back to the pinned");
  gBrowser.selectTabAtIndex(2);
  is(gBrowser.selectedTab, pinned, "no third tab, so no change");
  gBrowser.selectTabAtIndex(-1);
  is(gBrowser.selectedTab, testTab, "last tab is the test tab");

  gBrowser.tabContainer.advanceSelectedTab(1, true);
  is(gBrowser.selectedTab, pinned, "wrapped around the end to pinned");
  gBrowser.tabContainer.advanceSelectedTab(1, true);
  is(gBrowser.selectedTab, testTab, "next to test tab");
  gBrowser.tabContainer.advanceSelectedTab(1, true);
  is(gBrowser.selectedTab, pinned, "next to pinned again");

  gBrowser.tabContainer.advanceSelectedTab(-1, true);
  is(gBrowser.selectedTab, testTab, "going backwards to last tab");
  gBrowser.tabContainer.advanceSelectedTab(-1, true);
  is(gBrowser.selectedTab, pinned, "next to pinned");
  gBrowser.tabContainer.advanceSelectedTab(-1, true);
  is(gBrowser.selectedTab, testTab, "next to test tab again");

  
  gBrowser.showOnlyTheseTabs(Array.slice(gBrowser.tabs));
  is(gBrowser.visibleTabs.length, 3, "all 3 tabs are visible again");

  
  gBrowser.selectedTab = pinned;
  gBrowser.showOnlyTheseTabs([testTab]);
  is(gBrowser.tabs[1], origTab, "make sure origTab is in the middle");
  is(origTab.hidden, true, "make sure it's hidden");
  gBrowser.removeTab(pinned);
  is(gBrowser.selectedTab, testTab, "making sure origTab was skipped");
  is(gBrowser.visibleTabs.length, 1, "only testTab is there");

  
  gBrowser.showOnlyTheseTabs([origTab]);
  is(gBrowser.visibleTabs.length, 2, "got 2 tabs");

  
  gBrowser.showOnlyTheseTabs([testTab]);
  visible = gBrowser.visibleTabs;
  is(visible.length, 1, "only the original tab is visible");
  is(visible[0], testTab, "it's the original tab");
  is(gBrowser.tabs.length, 2, "still have 2 open tabs");

  
  gBrowser.removeTab(testTab);
  is(gBrowser.visibleTabs.length, 1, "only orig is left and visible");
  is(gBrowser.tabs.length, 1, "sanity check that it matches");
  is(gBrowser.selectedTab, origTab, "got the orig tab");
  is(origTab.hidden, false, "and it's not hidden -- visible!");

  if (tabViewWindow)
    tabViewWindow.GroupItems.groupItems[0].close();
}
