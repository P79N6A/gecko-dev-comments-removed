


function test() {
  waitForExplicitFinish();

  let origTab = gBrowser.visibleTabs[0];
  let newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  let relatedTab = gBrowser.addTab("about:blank", { ownerTab: newTab });

  
  
  TabView._initFrame(function() {
    let newTabGroupItemId = newTab._tabViewTabItem.parent.id;

    is(relatedTab.owner, newTab, "The related tab's owner is the right tab");

    
    TabView.moveTabTo(newTab, null);

    
    gBrowser.removeTab(relatedTab);

    is(gBrowser.visibleTabs.length, 1, "The number of visible tabs is 1");
    is(gBrowser.visibleTabs[0], origTab, 
      "The original tab is the only visible tab");
    isnot(newTab._tabViewTabItem.parent.id, newTabGroupItemId, 
      "The moved tab item has a new group id");

    
    gBrowser.removeTab(newTab);

    finish();
  });
}
