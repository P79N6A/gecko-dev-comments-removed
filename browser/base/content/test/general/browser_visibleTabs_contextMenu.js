



function test() {
  
  let [origTab] = gBrowser.visibleTabs;
  is(gBrowser.visibleTabs.length, 1, "there is one visible tab");
  let testTab = gBrowser.addTab();
  is(gBrowser.visibleTabs.length, 2, "there are now two visible tabs");

  
  updateTabContextMenu(origTab);
  is(document.getElementById("context_closeTab").disabled, false, "Close Tab is enabled");
  is(document.getElementById("context_reloadAllTabs").disabled, false, "Reload All Tabs is enabled");

  
  gBrowser.selectedTab = testTab;
  gBrowser.showOnlyTheseTabs([testTab]);
  is(gBrowser.visibleTabs.length, 1, "now there is only one visible tab");
  
  
  updateTabContextMenu(testTab);
  is(document.getElementById("context_closeTab").disabled, false, "Close Tab is enabled when more than one tab exists");
  is(document.getElementById("context_reloadAllTabs").disabled, true, "Reload All Tabs is disabled");
  
  
  
  let pinned = gBrowser.addTab();
  gBrowser.pinTab(pinned);
  is(gBrowser.visibleTabs.length, 2, "now there are two visible tabs");

  
  updateTabContextMenu(testTab);
  is(document.getElementById("context_closeOtherTabs").disabled, true, "Close Other Tabs is disabled");
  is(document.getElementById("context_closeTabsToTheEnd").disabled, true, "Close Tabs To The End is disabled");

  
  let allTabs = [tab for each (tab in gBrowser.tabs)];
  gBrowser.showOnlyTheseTabs(allTabs);

  
  updateTabContextMenu(testTab);
  is(document.getElementById("context_closeOtherTabs").disabled, false, "Close Other Tabs is enabled");
  is(document.getElementById("context_closeTabsToTheEnd").disabled, true, "Close Tabs To The End is disabled");
  
  
  
  updateTabContextMenu(origTab);
  is(document.getElementById("context_closeTabsToTheEnd").disabled, false, "Close Tabs To The End is enabled");

  gBrowser.removeTab(testTab);
  gBrowser.removeTab(pinned);
}
