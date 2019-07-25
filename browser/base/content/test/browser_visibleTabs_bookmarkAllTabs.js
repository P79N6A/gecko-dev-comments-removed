




































function test() {
  waitForExplicitFinish();

  
  let [origTab] = gBrowser.visibleTabs;
  is(gBrowser.visibleTabs.length, 1, "1 tab should be open");  
  is(Disabled(), true, "Bookmark All Tabs should be disabled");

  
  let testTab1 = gBrowser.addTab();
  is(gBrowser.visibleTabs.length, 2, "2 tabs should be open");
  is(Disabled(), true, "Bookmark All Tabs should be disabled since there are two tabs with the same address");

  let testTab2 = gBrowser.addTab("about:robots");
  is(gBrowser.visibleTabs.length, 3, "3 tabs should be open");
  
  testTab2.linkedBrowser.addEventListener("load", function () {
    testTab2.linkedBrowser.removeEventListener("load", arguments.callee, true);
    is(Disabled(), false, "Bookmark All Tabs should be enabled since there are two tabs with different addresses");

    
    gBrowser.selectedTab = testTab2;
    gBrowser.showOnlyTheseTabs([testTab2]);
    is(gBrowser.visibleTabs.length, 1, "1 tab should be visible");  
    is(Disabled(), true, "Bookmark All Tabs should be disabled as there is only one visible tab");

    
    let pinned = gBrowser.addTab();
    is(gBrowser.visibleTabs.length, 2, "2 tabs should be visible now");
    is(Disabled(), false, "Bookmark All Tabs should be available as there are two visible tabs");
    gBrowser.pinTab(pinned);
    is(Hidden(), false, "Bookmark All Tabs should be visible on a normal tab");
    is(Disabled(), true, "Bookmark All Tabs should not be available since one tab is pinned");
    gBrowser.selectedTab = pinned;
    is(Hidden(), true, "Bookmark All Tabs should be hidden on a pinned tab");

    
    let allTabs = [tab for each (tab in gBrowser.tabs)];
    gBrowser.showOnlyTheseTabs(allTabs);

    
    gBrowser.removeTab(testTab2);
    gBrowser.removeTab(testTab1);
    gBrowser.removeTab(pinned);
    is(gBrowser.visibleTabs.length, 1, "only orig is left and visible");
    is(gBrowser.tabs.length, 1, "sanity check that it matches");
    is(Disabled(), true, "Bookmark All Tabs should be hidden");
    is(gBrowser.selectedTab, origTab, "got the orig tab");
    is(origTab.hidden, false, "and it's not hidden -- visible!");
    finish();
  }, true);
}

function Disabled() {
  document.popupNode = gBrowser.selectedTab;
  TabContextMenu.updateContextMenu(document.getElementById("tabContextMenu"));
  TabContextMenu.contextTab = null;
  return document.getElementById("Browser:BookmarkAllTabs").getAttribute("disabled") == "true";
}

function Hidden() {
  document.popupNode = gBrowser.selectedTab;
  TabContextMenu.updateContextMenu(document.getElementById("tabContextMenu"));
  TabContextMenu.contextTab = null;
  return document.getElementById("context_bookmarkAllTabs").hidden;
}
