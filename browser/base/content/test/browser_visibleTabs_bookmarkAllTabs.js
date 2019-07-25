




































function test() {
  
  let [origTab] = gBrowser.visibleTabs;
  is(gBrowser.visibleTabs.length, 1, "1 tab should be open");  
  is(Disabled(), true, "Bookmark All Tabs should be hidden");

  
  let testTab = gBrowser.addTab();
  is(gBrowser.visibleTabs.length, 2, "2 tabs should be open");
  is(Disabled(), false, "Bookmark All Tabs should be available");

  
  gBrowser.selectedTab = testTab;
  gBrowser.showOnlyTheseTabs([testTab]);
  is(gBrowser.visibleTabs.length, 1, "1 tab should be visible");  
  is(Disabled(), true, "Bookmark All Tabs should be hidden as there is only one visible tab");
  
  
  let pinned = gBrowser.addTab();
  gBrowser.pinTab(pinned);
  is(gBrowser.visibleTabs.length, 2, "2 tabs should be visible now");
  is(Disabled(), false, "Bookmark All Tabs should be available as there are two visible tabs");

  
  let allTabs = [tab for each (tab in gBrowser.tabs)];
  gBrowser.showOnlyTheseTabs(allTabs);

  
  gBrowser.removeTab(testTab);
  gBrowser.removeTab(pinned);
  is(gBrowser.visibleTabs.length, 1, "only orig is left and visible");
  is(gBrowser.tabs.length, 1, "sanity check that it matches");
  is(Disabled(), true, "Bookmark All Tabs should be hidden");
  is(gBrowser.selectedTab, origTab, "got the orig tab");
  is(origTab.hidden, false, "and it's not hidden -- visible!");
}

function Disabled() {
  let command = document.getElementById("Browser:BookmarkAllTabs");
  return command.hasAttribute("disabled") && command.getAttribute("disabled") === "true";
}