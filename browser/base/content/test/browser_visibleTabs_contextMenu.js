




































function test() {
  
  let [origTab] = gBrowser.visibleTabs;
  is(gBrowser.visibleTabs.length, 1, "there is one visible tab");
  let testTab = gBrowser.addTab();
  is(gBrowser.visibleTabs.length, 2, "there are now two visible tabs");

  
  popup(origTab);
  is(TabContextMenu.contextTab, origTab, "TabContextMenu context is the original tab");
  is(document.getElementById("context_closeTab").disabled, false, "Close Tab is enabled");
  is(document.getElementById("context_reloadAllTabs").disabled, false, "Reload All Tabs is enabled");

  
  gBrowser.selectedTab = testTab;
  gBrowser.showOnlyTheseTabs([testTab]);
  is(gBrowser.visibleTabs.length, 1, "now there is only one visible tab");
  
  
  popup(testTab);
  is(TabContextMenu.contextTab, testTab, "TabContextMenu context is the test tab");
  is(document.getElementById("context_closeTab").disabled, false, "Close Tab is enabled when more than one tab exists");
  is(document.getElementById("context_reloadAllTabs").disabled, true, "Reload All Tabs is disabled");
  
  
  
  let pinned = gBrowser.addTab();
  gBrowser.pinTab(pinned);
  is(gBrowser.visibleTabs.length, 2, "now there are two visible tabs");

  
  popup(testTab);
  is(TabContextMenu.contextTab, testTab, "TabContextMenu context is again the test tab");
  is(document.getElementById("context_closeOtherTabs").disabled, true, "Close Other Tabs is disabled");

  
  let allTabs = [tab for each (tab in gBrowser.tabs)];
  gBrowser.showOnlyTheseTabs(allTabs);

  
  popup(testTab);
  is(TabContextMenu.contextTab, testTab, "TabContextMenu context is yet again the test tab");
  is(document.getElementById("context_closeOtherTabs").disabled, false, "Close Other Tabs is enabled");
  
  gBrowser.removeTab(testTab);
  gBrowser.removeTab(pinned);
}

function popup(tab) {
  document.popupNode = tab;
  TabContextMenu.updateContextMenu(document.getElementById("tabContextMenu"));
}